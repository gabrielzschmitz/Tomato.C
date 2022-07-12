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
  <a href="#-todo">Todo</a> •
  <a href="#-dependencie">Dependencie</a> •
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
$ tomatoc
```

To run it at the best terminal resolution use [setsid](https://man7.org/linux/man-pages/man1/setsid.1.html) (the geometry depends on your font size):
```
$ setsid -f "$TERMINAL" -g 25x14 -c tomato -e tomato
```

## 🍅 The Pomodoro Method
It consists of using a timer to break down work into <b>intervals</b>, follow the <b>steps</b>:
<i>
 1. <b>First 3 rounds:</b>
    - <b>25</b> minutes of <b>focus</b>;
    - <b>5</b> minutes of <b>break</b>;
 2. <b>Final round:</b>
    - <b>25</b> minutes of <b>focus</b>;
    - <b>30</b> minutes of <b>break</b>;
 3. <b>Repeat.</b>
</i>

## 📝 Todo
- [X] Make a welcome screen
- [ ] Rewrite using ncurses
- [ ] Make it auto center (without setsid)

## ⚓ Dependencie
It only needs [gcc](https://gcc.gnu.org/) to compile.

## 📜 License
This software is licensed under the [GPL-3](./LICENSE) license.

