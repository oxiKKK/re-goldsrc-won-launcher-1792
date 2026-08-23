# wonserver — local WON server emulator

`wonserverd.exe` implements the network services used by the WON Half-Life
launcher, build 1792. The launcher reaches it through its normal socket paths;
no emulator code is linked into `hl.exe`.

## Implemented launcher surface

| Launcher path | Transport | Request / reply |
|---|---|---|
| WON authentication | TCP Titan | svc 202: public keys, login challenge, certificate |
| Authenticated session | TCP Titan | svc 203 peer login and Blowfish session resume |
| Directory | TCP Titan | svc 30 public rooms and factory discovery |
| Chat factory | TCP Titan | svc 10 room creation, including passwords |
| Chat room | TCP Titan | svc 50 join, roster, joins/leaves, text relay |
| Room status | UDP | player count and find-player probes |
| Master bootstrap | UDP | `v` / `w` Auth, Titan, and Master lists |
| Internet server list | UDP | `e` or `1` / `f` server batches |
| Game server browser | UDP | ping, infostring, players, and rules |
| LAN browser | UDP broadcast | infostring replies on ports 27023 and 27024 |
| Mod catalog | UDP | `n` / `o` catalog and `p` install notification |
| Mod statistics | TCP | `x` / `y` server and player counts |

The default process exposes these ports:

- TCP 6002: Auth, peer auth, directory, and factory.
- UDP and TCP 27010: master queries and mod statistics.
- UDP 27011: mod catalog.
- TCP and UDP 27015–27022: default chat rooms.
- UDP 27023–27030: mock GoldSrc game servers.
- TCP 27100–27115: factory-created chat rooms.

## Configure the launcher

Point every block in the Half-Life folder's `woncomm.lst` at the emulator:

```
Titan
{
    127.0.0.1:6002
}
Auth
{
    127.0.0.1:6002
}
Master
{
    127.0.0.1:27010
}
ModServer
{
    127.0.0.1:27011
}
```

On its first start, the server generates `verifier.key`, `auth.key`, and
`kver.kp`. Key generation can take about a minute. Copy the generated `kver.kp`
into the Half-Life folder, replacing the original WON trust root. The launcher
cannot verify the emulator's certificates without that file.

Run from the directory in which the keys should be kept:

```
wonserverd.exe
```

Optional arguments are `-port N`, `-masterport N`, `-modport N`, `-keydir DIR`,
and `-noauth`.

## Build

The top-level build includes `wonserverd` and the compatibility tools. A
standalone 32-bit build is also supported:

```
cmake -S wonserver -B build/wonserver-standalone -A Win32
cmake --build build/wonserver-standalone --config Debug
```

`wonserverd.exe` is statically linked and imports only Windows system DLLs.
`wondirtest.exe` uses the reconstructed client DLL boundary, so CMake copies
`WONAuth.dll` and `WONCrypt.dll` beside it automatically.

## Verification

With `wonserverd` running:

```
wonprotocoltest.exe
wondirtest.exe 127.0.0.1 6002
wonauthtest.exe kver.kp 127.0.0.1 6002
```

`wonprotocoltest` drives the launcher wire formats over real TCP and UDP sockets:
master bootstrap and batches, all four game queries, both mod protocols, both
directories, factory room creation, password retry, room roster, find-player,
and chat relay. `wonauthtest` additionally verifies the signed key block and
certificate, completes svc 203, resumes the session on a fresh connection, and
accepts an encrypted directory reply.
