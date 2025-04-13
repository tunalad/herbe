# 🌱 herbe (tunalad's EPIC fork)

> Daemon-less notifications without D-Bus. Minimal and lightweight.

<p align="center">
  <img src="https://user-images.githubusercontent.com/24730635/90975811-cd62fd00-e537-11ea-9169-92e68a71d0a0.gif" />
</p>

This fork has some patches applied and other PRs that fix some stuff.

I also have done some other changes myself and will do a lot more, since I want to completely replace notify-send with herbe. I will probably rename this project later, since it probably won't be as small as it is right now.

## Features

- Doesn't run in the background, just displays the notification and exits
- Configurable through `config.h`
- [Actions support](#actions)
- Extensible through [patches](https://github.com/dudik/herbe/pulls?q=is%3Aopen+is%3Apr+label%3Apatch)

## Table of contents

- [Usage](#usage)
    - [Dismiss a notification](#dismiss-a-notification)
    - [Actions](#actions)
    - [Newlines](#newlines)
    - [Multiple notifications](#multiple-notifications)
- [Installation](#installation)
    - [Dependencies](#dependencies)
    - [Build](#build)
- [Configuration](#configuration)

## Usage

### Patches

Applied patches:

- [dynamically updated the displayed notification](https://github.com/dudik/herbe/pull/25)
- [Vertical stacking](https://github.com/dudik/herbe/pull/19)

Applied PRs that fixes stuff:

- [Various fixes/cleanups](https://github.com/dudik/herbe/pull/39)
- [don't call signal unsafe functions in sighandler](https://github.com/dudik/herbe/pull/40)
- [Fix linebreak for unicode symbols](https://github.com/dudik/herbe/pull/47)
- [Set window masks during window creation](https://github.com/dudik/herbe/pull/29)
- [Refactoring line parsing code to its own method](https://github.com/dudik/herbe/pull/24)

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

Notifications are stacked onto each other. We can override existing notifications by using `HERBE_ID` variables. Here's an example:

```shell
$ HERBE_ID=/1 ./herbe "First" "This notification will be overriden" & sleep 1 && HERBE_ID=/1 ./herbe "Second" "takes the place" & ./herbe "Third" "and unrelated"
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
$ git clone https://github.com/dudik/herbe
$ cd herbe
$ sudo make install
```

`make install` requires root privileges because it copies the resulting binary to `/usr/local/bin`. This makes `herbe` accessible globally.

You can also use `make clean` to remove the binary from the build folder, `sudo make uninstall` to remove the binary from `/usr/local/bin` or just `make` to build the binary locally.

## Configuration

herbe is configured at compile-time by editing `config.h`. Every option should be self-explanatory. There is no `height` option because height is determined by font size and text padding.

## Projects with herbe integration

- [qutebrowser](https://qutebrowser.org/) supports showing web notifications via herbe, via the `content.notifications.presenter` setting.
