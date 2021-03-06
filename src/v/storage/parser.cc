// Copyright 2020 Vectorized, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

#include "storage/parser.h"

#include "bytes/iobuf.h"
#include "bytes/iobuf_parser.h"
#include "likely.h"
#include "model/record.h"
#include "model/record_utils.h"
#include "reflection/adl.h"
#include "storage/logger.h"
#include "storage/parser.h"
#include "storage/parser_utils.h"
#include "vlog.h"

#include <seastar/core/smp.hh>
#include <seastar/util/variant_utils.hh>

#include <bits/stdint-uintn.h>
#include <fmt/format.h>

#include <algorithm>

namespace storage {
using stop_parser = batch_consumer::stop_parser;
using skip_batch = batch_consumer::skip_batch;

model::record_batch_header header_from_iobuf(iobuf b) {
    iobuf_parser parser(std::move(b));
    auto header_crc = reflection::adl<uint32_t>{}.from(parser);
    auto sz = reflection::adl<int32_t>{}.from(parser);
    using offset_t = model::offset::type;
    auto off = model::offset(reflection::adl<offset_t>{}.from(parser));
    auto type = reflection::adl<model::record_batch_type>{}.from(parser);
    auto crc = reflection::adl<int32_t>{}.from(parser);
    using attr_t = model::record_batch_attributes::type;
    auto attrs = model::record_batch_attributes(
      reflection::adl<attr_t>{}.from(parser));
    auto delta = reflection::adl<int32_t>{}.from(parser);
    using tmstmp_t = model::timestamp::type;
    auto first = model::timestamp(reflection::adl<tmstmp_t>{}.from(parser));
    auto max = model::timestamp(reflection::adl<tmstmp_t>{}.from(parser));
    auto producer_id = reflection::adl<int64_t>{}.from(parser);
    auto producer_epoch = reflection::adl<int16_t>{}.from(parser);
    auto base_sequence = reflection::adl<int32_t>{}.from(parser);
    auto record_count = reflection::adl<int32_t>{}.from(parser);
    vassert(
      parser.bytes_consumed() == model::packed_record_batch_header_size,
      "Error in header parsing. Must consume:{} bytes, but consumed:{}",
      model::packed_record_batch_header_size,
      parser.bytes_consumed());
    auto hdr = model::record_batch_header{
      .header_crc = header_crc,
      .size_bytes = sz,
      .base_offset = off,
      .type = type,
      .crc = crc,
      .attrs = attrs,
      .last_offset_delta = delta,
      .first_timestamp = first,
      .max_timestamp = max,
      .producer_id = producer_id,
      .producer_epoch = producer_epoch,
      .base_sequence = base_sequence,
      .record_count = record_count};
    hdr.ctx.owner_shard = ss::this_shard_id();
    return hdr;
}

static ss::future<result<iobuf>> verify_read_iobuf(
  ss::input_stream<char>& in, size_t expected, ss::sstring msg) {
    return read_iobuf_exactly(in, expected)
      .then([msg = std::move(msg), expected](iobuf b) {
          if (likely(b.size_bytes() == expected)) {
              return ss::make_ready_future<result<iobuf>>(std::move(b));
          }
          stlog.error(
            "Cannot continue parsing. recived size:{} bytes, expected:{} "
            "bytes. context:{}",
            b.size_bytes(),
            expected,
            msg);
          return ss::make_ready_future<result<iobuf>>(
            parser_errc::input_stream_not_enough_bytes);
      });
}

ss::future<result<stop_parser>> continuous_batch_parser::consume_header() {
    return read_iobuf_exactly(_input, model::packed_record_batch_header_size)
      .then([this](iobuf b) -> result<iobuf> {
          if (b.empty()) {
              // benign outcome. happens at end of file
              return parser_errc::end_of_stream;
          }
          if (b.size_bytes() != model::packed_record_batch_header_size) {
              stlog.error(
                "Could not parse header. Expected:{}, but Got:{}. consumer:{}",
                model::packed_record_batch_header_size,
                b.size_bytes(),
                *_consumer);
              return parser_errc::input_stream_not_enough_bytes;
          }
          return b;
      })
      .then([](result<iobuf> b) {
          if (!b) {
              return ss::make_ready_future<result<model::record_batch_header>>(
                b.error());
          }
          return ss::make_ready_future<result<model::record_batch_header>>(
            header_from_iobuf(std::move(b.value())));
      })
      .then([this](result<model::record_batch_header> o) {
          if (!o) {
              return ss::make_ready_future<result<stop_parser>>(o.error());
          }
          if (o.value().header_crc == 0) {
              // happens when we fallocate the file
              return ss::make_ready_future<result<stop_parser>>(
                parser_errc::end_of_stream);
          }
          if (auto computed_crc = model::internal_header_only_crc(o.value());
              unlikely(o.value().header_crc != computed_crc)) {
              vlog(
                stlog.error,
                "detected header corruption. stopping parser. Expected CRC of "
                "{}, but got header CRC: {} - {}. consumer:{}",
                computed_crc,
                o.value().header_crc,
                o.value(),
                *_consumer);
              return ss::make_ready_future<result<stop_parser>>(
                parser_errc::header_only_crc_missmatch);
          }
          if (unlikely(o.value().header_crc == 0)) {
              return ss::make_ready_future<result<stop_parser>>(
                parser_errc::fallocated_file_read_zero_bytes_for_header);
          }
          _header = o.value();
          const auto size_on_disk = _header.size_bytes;
          auto ret = _consumer->consume_batch_start(
            _header, _physical_base_offset, size_on_disk);
          _physical_base_offset += size_on_disk;
          if (std::holds_alternative<skip_batch>(ret)) {
              auto s = std::get<skip_batch>(ret);
              if (unlikely(bool(s))) {
                  auto remaining = _header.size_bytes
                                   - model::packed_record_batch_header_size;
                  return verify_read_iobuf(
                           _input, remaining, "parser::skip_batch")
                    .then([this](result<iobuf> b) {
                        if (!b) {
                            return ss::make_ready_future<result<stop_parser>>(
                              b.error());
                        }
                        // start again
                        add_bytes_and_reset();
                        return consume_header();
                    });
              }
              return ss::make_ready_future<result<stop_parser>>(
                stop_parser::no);
          }
          auto s = std::get<stop_parser>(ret);
          if (unlikely(bool(s))) {
              return ss::make_ready_future<result<stop_parser>>(
                stop_parser::yes);
          }
          return ss::make_ready_future<result<stop_parser>>(stop_parser::no);
      });
}

ss::future<result<stop_parser>> continuous_batch_parser::consume_one() {
    return consume_header().then([this](result<stop_parser> st) {
        if (!st) {
            return ss::make_ready_future<result<stop_parser>>(st.error());
        }
        if (st.value() == stop_parser::yes) {
            return ss::make_ready_future<result<stop_parser>>(st.value());
        }
        return consume_records();
    });
}

size_t continuous_batch_parser::consumed_batch_bytes() const {
    return _header.size_bytes;
}

void continuous_batch_parser::add_bytes_and_reset() {
    _bytes_consumed += consumed_batch_bytes();
    _header = {}; // reset
}
ss::future<result<stop_parser>> continuous_batch_parser::consume_records() {
    auto sz = _header.size_bytes - model::packed_record_batch_header_size;
    return verify_read_iobuf(_input, sz, "parser::consume_records")
      .then([this](result<iobuf> record) -> result<stop_parser> {
          if (!record) {
              return record.error();
          }
          _consumer->consume_records(std::move(record.value()));
          return result<stop_parser>(_consumer->consume_batch_end());
      });
}

ss::future<result<size_t>> continuous_batch_parser::consume() {
    if (unlikely(_err != parser_errc::none)) {
        return ss::make_ready_future<result<size_t>>(_err);
    }
    return ss::repeat([this] {
               return consume_one().then([this](result<stop_parser> s) {
                   add_bytes_and_reset();
                   if (_input.eof()) {
                       return ss::stop_iteration::yes;
                   }
                   if (!s) {
                       _err = parser_errc(s.error().value());
                       return ss::stop_iteration::yes;
                   }
                   if (s.value() == stop_parser::yes) {
                       return ss::stop_iteration::yes;
                   }
                   return ss::stop_iteration::no;
               });
           })
      .then([this] {
          if (_bytes_consumed) {
              // support partial reads
              return result<size_t>(_bytes_consumed);
          }
          constexpr std::array<parser_errc, 3> benign_error_codes{
            {parser_errc::none,
             parser_errc::end_of_stream,
             parser_errc::fallocated_file_read_zero_bytes_for_header}};
          if (std::any_of(
                benign_error_codes.begin(),
                benign_error_codes.end(),
                [v = _err](parser_errc e) { return e == v; })) {
              return result<size_t>(_bytes_consumed);
          }
          return result<size_t>(_err);
      });
}
} // namespace storage
