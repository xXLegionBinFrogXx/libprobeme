# Fixtures

Committed `/proc` snapshots from real hosts, one directory per
`<arch>/<kernel>`, captured with `tests/fixtures/capture.sh`:

```
tests/fixtures/linux/<arch>/<kernel>/proc/{stat,meminfo,loadavg,uptime,diskstats,net_dev,self_mounts}
```

Unit tests iterate every committed fixture directory, so new captures are
picked up automatically. Without any fixtures, tests still cover inline
malformed-input cases.
