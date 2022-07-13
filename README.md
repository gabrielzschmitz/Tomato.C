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
  <a href="#-how-to-install">How to Install</a> •
  <a href="#-how-to-use">How to Use</a> •
  <a href="#-the-pomodoro-method">The Pomodoro Method</a> •
  <a href="#-to-do">To-do</a> •
  <a href="#-dependencies">Dependencies</a> •
  <a href="#-license">License</a>
</p>

## 💾 How to Install
<b>Note</b>: a good practice is to clone the repo at <i>$HOME/.local/src/</i>.
```
$ git clone https://github.com/gabrielzschmitz/Tomato.C.git
$ cd Tomato.C
$ sudo make
```

## 🚀 How to Use
```
$ tomato
```

To run it at the best terminal resolution use [setsid](https://man7.org/linux/man-pages/man1/setsid.1.html) (the geometry depends on your font size):
```
$ setsid -f "$TERMINAL" -g 25x14 -c tomato -e tomato
```

## 🍅 The Pomodoro Method
<img src="./media/tomatomethod.gif" alt="tomatomethod" width="210px" align="right">

The technique basically consists of using a timer to break down work into <b>intervals</b>, follow the <b>steps</b>:

 1. Get a <b>to-do list</b>;

 2. Start [Tomato.C](https://github.com/gabrielzschmitz/Tomato.C) and focus on a <b>single task for 25 minutes</b> straight until notification pops up;

 3. Then <b>record what you completed</b> and enjoy a <b>5 minutes break</b>;

 4. After <b>4 pomodoros</b> (steps 2 and 3), take a longer, <b>30 minutes break</b>;

 5. <b>Restart</b>.

## 📝 To-do
- [X] Make a welcome screen
- [ ] Implement input controls
- [ ] Implement user options
- [ ] Rewrite using ncurses
- [ ] Make it auto center (without setsid)

## ⚓ Dependencies
It only needs [gcc](https://gcc.gnu.org/) to compile, and [dunst](https://github.com/dunst-project/dunst) to show notifications.

## 📜 License
This software is licensed under the [GPL-3](./LICENSE) license.

