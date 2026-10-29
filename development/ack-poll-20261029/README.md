# Fullscreen ACK polling experiment

A 250 microsecond minimum interval between pending-flip acknowledgement reads.
The first poll after queueing is immediate. The existing lock, ACK counter,
timeout, cancellation and reset-before-free paths remain authoritative.
No sleeping under the bitmap lock and no firmware or shared-memory ABI change.
Windowed operation follows the original path.

The interval is an opt-in host experiment, not a release default. Build with:

```sh
python3 scripts/build_current.py --expanded --jit-registers --ack-poll-us 250
```

The separate output drawer ends in `-ack250`. Builds never connect or deploy.
`player/ack-poll.json` records source/payload hashes; `ack-poll.patch` records
exact changes. Do not combine other experiments when comparing performance.

Native policy test:

```sh
cc -std=c99 -Wall -Wextra -Werror development/ack-poll-20261029/test_poll.c -o /tmp/zz-ack-test
/tmp/zz-ack-test
python3 scripts/test_ack_poll_host.py <complete-build/player> --interval 250
```

Host tests execute the staged retirement/cancellation functions against mocked
registers. They do not establish firmware or optical scanout behavior.
See [physical results](../../docs/ack-polling.md).
