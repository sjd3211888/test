# Discovery Server Usage

```SQL
Usage: discovery_server <entity> [options]

Entities:
  publisher                            Run a client publisher entity.
  subscriber                           Run a client subscriber entity.
  server                               Run a server entity.

  -h,       --help                     Print this help message.
Client options (common to Publisher, Subscriber and Server acting as Client):
  -c <str>, --connection-locator <str> Address of the Server to connect to
                                       (Default address: udpv4:127.0.0.1:16166, this option can be configured multiple times.).
                                       [udpv4:A.B.C.D:PORT|uds:/path/file]

Publisher options:
  -t <str>, --topic <str>              Topic name
                                       (Default: discovery_server_topic).
  -r, --reliable                       Set Reliability QoS as reliable
                                       (Default: best effort)
      --transient-local                Set Durability QoS as transient local
                                       (Default: volatile)
  -s <num>, --samples <num>            Number of samples to send
                                       (Default: 0 => infinite samples).
  -i <num>, --interval <num>           Time between samples in milliseconds
                                       (Default: 100).

Subscriber options:
  -t <str>, --topic <str>              Topic name
                                       (Default: discovery_server_topic).
  -s <num>, --samples <num>            Number of samples to receive
                                       (Default: 0 => infinite samples).
  -r, --reliable                       Set Reliability QoS as reliable
                                       (Default: best effort)
      --transient-local                Set Durability QoS as transient local
                                       (Default: volatile)

Server options:
            --mode <str>               Mode of server
                                       (Default: all).
                                       [all|rpc|dds|agent]
            --listening-locator <str>  Server listening address
                                       (Default address: udpv4:127.0.0.1:16166, this option can be configured multiple times.).
                                       [udpv4:A.B.C.D:PORT|uds:/path/file]
            --timeout <num>            Number of seconds before finish
                                       the process (Default: 0 = till ^C).
```
