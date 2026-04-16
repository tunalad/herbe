# 🌱 herbe (tunalad's EPIC fork)

> Daemon-less notifications without D-Bus. Minimal and lightweight.

<p align="center">
  <img src="https://user-images.githubusercontent.com/24730635/90975811-cd62fd00-e537-11ea-9169-92e68a71d0a0.gif" />
</p>

This fork builds on top of the original [dudik/herbe](https://github.com/dudik/herbe) with additional features to fully replace `notify-send`:

- Urgency levels (low, normal, critical) with different color schemes
- Configurable timeout via CLI
- Notification replacement by ID
- Optional wait mode (run in foreground)

## Features

- Doesn't run in the background by default (daemon-less without D-Bus)
- Configurable through `config.h`
- [Actions support](#actions)
- Vertical stacking support
- Font fallbacking for emoji/unicode support
- Extensible through [patches](https://github.com/dudik/herbe/pulls?q=is%3Aopen+is%3Apr+label%3Apatch)

## Table of contents

- [Usage](#usage)
    - [Command-line options](#command-line-options)
    - [Examples](#examples)
    - [Urgency levels](#urgency-levels)
    - [Dismiss a notification](#dismiss-a-notification)
    - [Actions](#actions)
    - [Newlines](#newlines)
    - [Multiple notifications](#multiple-notifications)
- [Installation](#installation)
    - [Dependencies](#dependencies)
    - [Build](#build)
- [Configuration](#configuration)
- [Roadmap / To-Do](#roadmap--to-do)

## Usage

### Command-line Options

| Short | Long | Description |
|-------|------|-------------|
| `-u` | `--urgency` | Set urgency level (low, normal, critical) |
| `-t` | `--expire-time` | Set display duration in seconds |
| `-r` | `--replace-id` | Set notification ID to replace |
| `-w` | `--wait` | Wait for notification to close before exiting |
| `-?` | `--help` | Show help text |
| `-v` | `--version` | Show version information |

### Examples

Display a notification with custom timeout:

```shell
$ herbe -t 10 "This notification will disappear after 10 seconds"
```

Display a critical urgency notification (stays until dismissed):

```shell
$ herbe -u critical "System emergency!"
```

Override an existing notification by ID:

```shell
$ herbe -r /myid "Updated content"
```

Or using the environment variable:

```shell
$ HERBE_ID=/myid herbe "Updated content"
```

Wait for the notification to be dismissed (run in foreground):

```shell
$ herbe -w "I'll block until dismissed"
```

### Urgency Levels

The `-u` option sets the urgency which affects the notification color:

| Level | Description | Default Color |
|-------|-------------|----------------|
| `low` | Low urgency | Dark gray |
| `normal` | Normal urgency | Blue |
| `critical` | Critical urgency (stays until dismissed) | Red |

### Dismiss a notification

A notification can be dismissed either by clicking on it with `DISMISS_BUTTON` (set in config.h, defaults to left mouse button) or sending a `SIGUSR1` signal to it:

```shell
$ pkill -SIGUSR1 herbe
```

Dismissed notifications return exit code 2.

### Actions

Action is a piece of shell code that runs when a notification gets accepted. Accepting a notification is the same as dismissing it, but you have to use either `ACTION_BUTTON` (defaults to right mouse button) or the `SIGUSR2` signal.
An accepted notification always returns exit code 0. To specify an action:

```shell
$ herbe "Notification body" && echo "This is an action"
```

Where everything after `&&` is the action and will get executed after the notification gets accepted.

### Newlines

Every command line argument gets printed on a separate line by default e.g.:

```shell
$ herbe "First line" "Second line" "Third line" ...
```

You can also use `\n` e.g. in `bash`:

```shell
$ herbe $'First line\nSecond line\nThird line'
```

But by default `herbe` prints `\n` literally:

```shell
$ herbe "First line\nStill the first line"
```

Output of other programs will get printed correctly, just make sure to escape it (so you don't end up with every word on a separate line):

```shell
$ herbe "$(ps axch -o cmd:15,%cpu --sort=-%cpu | head)"
```

### Multiple notifications

Notifications stack vertically onto each other. You can replace an existing notification by using the same ID:

```shell
$ ./herbe -r /1 "First notification" & sleep 1 && ./herbe -r /1 "This replaces the first"
```

The ID can also be set via the `HERBE_ID` environment variable (the `/` prefix is added automatically if missing):

```shell
$ HERBE_ID=myid ./herbe "First" & sleep 1 && HERBE_ID=myid ./herbe "Updated"
```

## Installation

### Dependencies

- X11 (Xlib)
- Xft
- fontconfig
- freetype2

The names of packages are different depending on which distribution you use.
For example, if you use [Void Linux](https://voidlinux.org/) you will have to install these dependencies:

```shell
$ sudo xbps-install base-devel libX11-devel libXft-devel fontconfig freetype
```

### Build

```shell
$ git clone https://github.com/tunalad/herbe
$ cd herbe
$ make
$ sudo make install
```

`make install` requires root privileges because it copies the resulting binary to `/usr/local/bin`. This makes `herbe` accessible globally.

You can also use `make clean` to remove the binary from the build folder, `sudo make uninstall` to remove the binary from `/usr/local/bin` or just `make` to build the binary locally.

## Configuration

herbe is configured at compile-time by editing `config.h`. Every option should be self-explanatory. There is no `height` option because height is determined by font size and text padding.

## Projects with herbe integration

- [qutebrowser](https://qutebrowser.org/) supports showing web notifications via herbe, via the `content.notifications.presenter` setting.

## Roadmap / To-Do

This fork aims to fully replace `notify-send`:

### Implemented

| Option | Status |
|--------|--------|
| `-u` / `--urgency` | ✅ Implemented (low, normal, critical) |
| `-t` / `--expire-time` | ✅ Implemented (in seconds) |
| `-r` / `--replace-id` | ✅ Implemented |
| `-w` / `--wait` | ✅ Implemented |
| `-?` / `--help` | ✅ Implemented |
| `-v` / `--version` | ✅ Implemented |
| Summary + Body | ✅ Multiple args = multiple lines |
| Actions | ✅ Via `&&` pattern |

### Not Yet Implemented

| Option | Status |
|--------|--------|
| `-a` / `--app-name` | 🔲 Not implemented |
| `-A` / `--action` | 🔲 Native action buttons |
| `-i` / `--icon` | 🔲 Icon support |
| `-c` / `--category` | 🔲 Category hints |
| `-h` / `--hint` | 🔲 Custom hints |
| `-p` / `--print-id` | 🔲 Print notification ID |
| `--id-fd` | 🔲 Write ID to file descriptor |
| `-e` / `--transient` | 🔲 Transient notifications |

### Known Differences from notify-send

- Timeout is in **seconds** (not milliseconds)
- Actions use shell pattern (`cmd && action`) instead of native buttons
- No D-Bus dependency (X11 only)
- Compile-time configuration via `config.h`
