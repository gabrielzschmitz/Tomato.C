<h1 align="center">
    <br>
    <img src="./media/tomato.gif" alt="tomatowelcome" width="175px">
    <img src="./media/coffee.gif" alt="tomatocoffee" width="175px">
    <img src="./media/machine.gif" alt="tomatomachine" width="175px">
    <img src="./media/beach.gif" alt="tomatobeach" width="175px">
    <br>
    Tomato.C
    <br>
</h1>

<h4 align="center">A pomodoro timer written in pure <a href="https://www.open-std.org/JTC1/SC22/WG14/www/standards" target="_blank">C</a>.</h4>

<p align="center">
<a href="./LICENSE"><img src="https://img.shields.io/badge/license-GPL-3.svg" alt="License"></a>
<a href="https://www.buymeacoffee.com/gabrielzschmitz" target="_blank"><img src="https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png" alt="Buy Me A Coffee" style="height: 20px !important;width: 87px;" ></a>
<a href="https://github.com/gabrielzschmitz/Tomato.C"><img src="https://img.shields.io/github/stars/gabrielzschmitz/Tomato.C?style=social" alt="Give me a Star"></a>
</p>

<p align="center">
  <a href="#-dependencies">Dependencies</a> •
  <a href="#-how-to-install">How to Install</a> •
  <a href="#-how-to-use">How to Use</a> •
  <a href="#%EF%B8%8F-controls">Controls</a> •
  <a href="#%EF%B8%8F-preferences">Preferences</a>
</p>

<p align="center">
  <a href="#-the-pomodoro-method">The Pomodoro Method</a> •
  <a href="#-to-do">To-do</a> •
  <a href="#-contribute">Contribute</a> •
  <a href="#-license">License</a>
</p>

## ⚓ Dependencies

It only needs [gcc](https://gcc.gnu.org/) to compile, [ncurses](https://invisible-island.net/ncurses/) as the graphic library and [pkg-config](https://github.com/freedesktop/pkg-config) to proper librarys linking.
But optionally you can install [dunst](https://github.com/dunst-project/dunst)(or other notification daemon with notify-send support) to show notifications, [mpv](https://mpv.io/) for the notifications sounds and a [Nerd Font](https://www.nerdfonts.com/) for the icons:

```
ARCH LINUX
$ sudo pacman -S base-devel ncurses mpv pkgconf
UBUNTU
$ sudo apt install build-essential libncurses5-dev libncursesw5-dev mpv pkg-config
FEDORA
$ sudo dnf groupinstall 'Development Tools' && sudo dnf install ncurses-devel mpv pkgconf
MACOS (MacPorts needed)
$ brew install gcc && sudo port install ncurses mpv
```

<b>Note</b>: if you're using <b>WSL</b>, install [wsl-notify-send](https://github.com/stuartleeks/wsl-notify-send) to get the notifications and then toggle it in the config.h. Saddly [mpv](https://mpv.io/) don't work at WSL, so there's not custom sounds.

## 💾 How to Install
<b>Note</b>: a good practice is to clone the repo at <i>$HOME/.local/src/</i>

<b>Note</b>: first install all the <i>dependencies</i>!

```
NIXOS:
$ git clone https://github.com/gabrielzschmitz/Tomato.C.git
$ cd Tomato.C
$ nix-build default.nix

NORMAL:
$ git clone https://github.com/gabrielzschmitz/Tomato.C.git
$ cd Tomato.C
$ sudo make install
```

## 🚀 How to Use

Just <b>type it</b> in the <b>terminal</b>:
```
$ tomato
```

<b>Tip:</b> For the best terminal resolution use [setsid](https://man7.org/linux/man-pages/man1/setsid.1.html) (the geometry depends on your font size):
```
$ setsid -f "$TERMINAL" -g 33x21 -c tomato -e tomato
```

## 🕹️ Controls
<img src="./media/preferences.gif" alt="preferences" width="190" align="right">

Use the following <b>keys</b> to <b>control</b> the application:
 * <b><i>Mouse:</i></b> To select and increase or decrease.
 * <b><i>Arrows or VIM Keys:</i></b> To move and select;
 * <b><i>ENTER:</i></b> To select;
 * <b><i>CTRL+X:</i></b> To return to the main menu wherever you are;
 * <b><i>P or CTRL+P:</i></b> To toggle pause;
 * <b><i>ESC or Q:</i></b> To quit.

## ⚙️ Preferences
You can configure the following settings at run time:

 * <b><i>Pomodoros Amount</i></b>;
 * <b><i>Work Time</i></b>;
 * <b><i>Short Pause Time</i></b>;
 * <b><i>Long Pause Time</i></b>.

And change the default configurations editing the [config.h](https://github.com/gabrielzschmitz/Tomato.C/blob/master/config.h), then `sudo make install` to take effect.
You can change those configs:

 * <b><i>WSL</i></b>: 0/1;
 * <b><i>ICONS</i></b>: iconsoff - iconson - nerdicons;
 * <b><i>NOTIFY</i></b>: 0/1;
 * <b><i>SOUND</i></b>: 0/1;
 * <b><i>POMODOROS</i></b>: 1-8;
 * <b><i>WORKTIME</i></b>: 5-50;
 * <b><i>SHORTPAUSE</i></b>: 1-10;
 * <b><i>LONGPAUSE</i></b>: 5-60.

## 🍅 The Pomodoro Method
<img src="./media/tomatomethod.gif" alt="tomatomethod" width="190px" align="right">

The technique basically consists of using a timer to break down work into <b>intervals</b>, follow the <b>steps</b>:

 1. Get a <b>to-do list</b>;

 2. Start [Tomato.C](https://github.com/gabrielzschmitz/Tomato.C) and focus on a <b>single task for 25 minutes</b> straight until notification pops up;

 3. Then <b>record what you completed</b> and enjoy a <b>5 minutes break</b>;

 4. After <b>4 pomodoros</b> (steps 2 and 3), take a longer, <b>30 minutes break</b>;

 5. <b>Restart</b>.

<b>Note</b>: the <b>timers</b> and the <b>amount of pomodoros can be changed</b> by the user.
## 📝 To-do
- [X] Make a welcome screen
- [X] Rewrite using ncurses
- [X] Implement input controls
- [X] Implement user options
- [X] Make it auto center
- [X] Add notifications
- [X] Add notifications sound
- [X] Implement mouse support
- [ ] Implement save current state
- [ ] Implement simple note taking (maybe using a nvim instance)

## 🤝 Contribute
Feel free to contribute to the project, the only requirement is to follow the commit tittle pattern:

 * File-Related-Emoji Tittle

## 📜 License
This software is licensed under the [GPL-3](./LICENSE) license.

