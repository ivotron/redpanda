## PRs that have empty release notes section
* Backport 30min default cloud storage upload interval by @RafalKorepta in https://github.com/vectorizedio/redpanda/pull/3241
* Backport #3205 #3245 by @mmaslankaprv in https://github.com/vectorizedio/redpanda/pull/3286
* Backport #3233 by @Lazin in https://github.com/vectorizedio/redpanda/pull/3248
* Backport #3372 (Delay prefix truncation of offset translator until segments are deleted) by @ztlpn in https://github.com/vectorizedio/redpanda/pull/3541
* Backport #3479 ( Limit remote partition concurrency) by @Lazin in https://github.com/vectorizedio/redpanda/pull/3510
* Backport "Fix archival upload data corruption" #3293 by @Lazin in https://github.com/vectorizedio/redpanda/pull/3309
* Backport PR #3348 by @Lazin in https://github.com/vectorizedio/redpanda/pull/3373
* cloud_storage: suppress warning on successfull topic creation by @LenaAn in https://github.com/vectorizedio/redpanda/pull/3484

## PRs that have 'none' or 'n/a' in the release notes section
* 21.11.x backport3371 by @LenaAn in https://github.com/vectorizedio/redpanda/pull/3379
* archival: backport #3508 to fix bug in logging by @LenaAn in https://github.com/vectorizedio/redpanda/pull/3512
* Backport pr3358 pr3361 to v21.11.x by @BenPope in https://github.com/vectorizedio/redpanda/pull/3375
* [v21.11.x] cloud_storage: ensure readers only destroyed in remote_partition by @jcsp in https://github.com/vectorizedio/redpanda/pull/3448
* [v21.11.x] cloud_storage: fix file descriptor leak by @jcsp in https://github.com/vectorizedio/redpanda/pull/3437

## PRs that have a single line in the release notes section
* Fixes a consistency issue with transactions by @rystsov in https://github.com/vectorizedio/redpanda/pull/3247
* kafka server send group topic partition offset metric to prometheus by @ZeDRoman in https://github.com/vectorizedio/redpanda/pull/3440

## PRs that have a multiple lines in the release notes section
* 21.11.X: k8s: fix bug reconciling clusters with fixed nodeport by @alenkacz in https://github.com/vectorizedio/redpanda/pull/3281
* backport 3308: Stop partition_record_batch_reader_impl by @Lazin in https://github.com/vectorizedio/redpanda/pull/3364
* Backport #3365 to 21.11.x by @ztlpn in https://github.com/vectorizedio/redpanda/pull/3434
* backport: 3425 (archival: Make upload loop more resilient) by @Lazin in https://github.com/vectorizedio/redpanda/pull/3441
* Backport pr3271 to v21.11.x (redpanda/cluster: Improve logging for leader_balancer) by @BenPope in https://github.com/vectorizedio/redpanda/pull/3284
* cloud_storage: backport pr3356 by @LenaAn in https://github.com/vectorizedio/redpanda/pull/3357
* k8s: use specified external port for kafka-api config by @alenkacz in https://github.com/vectorizedio/redpanda/pull/3387
* v21.11.x backport pr3214 by @BenPope in https://github.com/vectorizedio/redpanda/pull/3222
* V21.11.x backport pr3280 by @LenaAn in https://github.com/vectorizedio/redpanda/pull/3343
* v21.11.x backport pr3299 by @BenPope in https://github.com/vectorizedio/redpanda/pull/3402
* [v21.11.x] bytes: impose max chunk size on ss_next_allocation_size by @jcsp in https://github.com/vectorizedio/redpanda/pull/3472
* [v21.11.x] kafka: limit bytes in many-partition fetch by @jcsp in https://github.com/vectorizedio/redpanda/pull/3438
* [v21.11.x] storage/cloud_storage: make read buffer sizes configurable by @jcsp in https://github.com/vectorizedio/redpanda/pull/3436

**Full Changelog**: https://github.com/vectorizedio/redpanda/compare/v21.11.2...v21.11.3
