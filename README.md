```
requirements:
    - compiler with GNU23 support
    - libcurl (dev)

build:
    run `make` (output in build/)

usage:
    [] - optional

    online [all]
        Lists servers with players in them.
        Pass `all` to include empty servers.

    rank <id>
        Shows the ranking information of players matching <id>
        A username matches any player whose name contains the given string
        (case-insensitive).
        Use the `@` prefix to look up a player by rank instead.

    lb [count] [start]
        Shows the current ranking leaderboard.
        [count] limits the players shown.
        [start] sets the starting rank.
```
