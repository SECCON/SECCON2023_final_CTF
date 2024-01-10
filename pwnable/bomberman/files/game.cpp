#include <iostream>
#include <memory>
#include <random>
#include <ncurses.h>
#define SEC *10

using u8  = unsigned char;
using i16 = short;
using i32 = int;
enum OBJECT_TYPE { OBJECT_EMPTY = 1, OBJECT_BOMB, OBJECT_FLAG, OBJECT_WALL, OBJECT_BLOCK };
enum GAME_STATE { IN_PROGRESS = 1, GAMEOVER, GOT_FLAG };
enum DIRECTION { LEFT = 1, RIGHT, UP, DOWN };

/* Bomb class */
struct Bomb {
  Bomb(i16 x, i16 y) : _x(x), _y(y), _timer(0) {}
  Bomb(Bomb* other) { other->x() = x(); other->y() = y(); }
  i16& x() { return _x; }
  i16& y() { return _y; }
  i32& timer() { return _timer; }

private:
  i32 _timer;
  i16 _x, _y;
};

/* Fire class (for animation) */
using Fire = Bomb;

/* Player class */
struct Player {
  Player(i16 x, i16 y) : _x(x), _y(y) { new_bomb(); }
  ~Player() { if (_bomb) _bomb.release(); }
  i16& x() { return _x; }
  i16& y() { return _y; }
  std::unique_ptr<Bomb>& bomb() { return _bomb; }
  bool has_bomb() { return _bomb.get() != nullptr; }
  void new_bomb() { if (!_bomb) _bomb = std::make_unique<Bomb>(_x, _y); }

private:
  i16 _x, _y;
  std::unique_ptr<Bomb> _bomb;
};

struct Stage {
  Stage(i16 width, i16 height)
    : _width(width), _height(height), _fire(nullptr), _state(IN_PROGRESS) {
    std::random_device rnd;

    /* Create player */
    _player = new Player(1, 1);

    /* Create field */
    _field = new u8[width * height]();
    // Randomly place blocks
    for (i16 y = 0; y < height; y++)
      for (i16 x = 0; x < width; x++)
        if (rnd() % 3UL == 0)
          at(x, y) = OBJECT_BLOCK;
    // Field around player should be empty
    at(1, 1) = at(1, 2) = at(2, 1) = OBJECT_EMPTY;
    // Outer wall
    for (i16 x = 0; x < width; x++)
      at(x, 0) = at(x, height-1) = OBJECT_WALL;
    for (i16 y = 0; y < height; y++)
      at(0, y) = at(width-1, y) = OBJECT_WALL;
    // Pillars
    for (i16 y = 2; y < height; y += 2)
      for (i16 x = 2; x < width; x += 2)
        at(x, y) = OBJECT_WALL;
    // The goal
    at(_width-3, _height-2) = at(_width-2, _height-3) = OBJECT_WALL;
    at(_width-2, _height-2) = OBJECT_FLAG;

    /* Setup screen */
    initscr();
    timeout(100);
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
  }

  ~Stage() {
    endwin();
    if (_bomb) _bomb.release();
    delete[] _field;
    delete _player;
  }

  bool is_gameover() { return _state != IN_PROGRESS; }
  u8& at(i16 x, i16 y) { return _field[y*_width + x]; }
  bool has_bomb() { return _bomb.get() != nullptr; }
  bool is_exploding() { return _fire != nullptr; }
  bool is_solid(i16 x, i16 y) {
    u8 obj = at(x, y);
    return obj == OBJECT_WALL || obj == OBJECT_BLOCK;
  }

  void move(enum DIRECTION dir) {
    /* Move player */
    if (dir == LEFT && !is_solid(_player->x()-1, _player->y()))
      _player->x()--;
    else if (dir == RIGHT && !is_solid(_player->x()+1, _player->y()))
      _player->x()++;
    else if (dir == UP && !is_solid(_player->x(), _player->y()-1))
      _player->y()--;
    else if (dir == DOWN && !is_solid(_player->x(), _player->y()+1))
      _player->y()++;

    if (_player->bomb()) {
      /* Sync bomb position with player position */
      _player->bomb().get()->x() = _player->x();
      _player->bomb().get()->y() = _player->y();
    }
  }

  void put_bomb() {
    if (!_player->has_bomb())
      return;

    /* Move bomb from player to stage */
    _bomb = std::move(_player->bomb());
    _bomb.get()->timer() = 0;

    /* Mark bomb on field */
    at(_bomb.get()->x(), _bomb.get()->y()) = OBJECT_BOMB;
  }

  void pickup_bomb() {
    if (has_bomb()
        && at(_player->x(), _player->y()) == OBJECT_BOMB) {
      /* Move bomb from stage to player */
      _player->bomb() = std::move(_bomb);
      at(_player->x(), _player->y()) = OBJECT_EMPTY;
    }
  }

  void tick() {
    /* Player got the flag */
    if (at(_player->x(), _player->y()) == OBJECT_FLAG) {
      _state = GOT_FLAG;
      return;
    }

    /* Extinguish fire */
    if (is_exploding() && _fire->timer()++ >= 1 SEC) {
      delete _fire;
      _fire = nullptr;
      _bomb.release();
      _player->new_bomb();

      /* Remove bomb from map */
      for (i16 y = 0; y < _height; y++)
        for (i16 x = 0; x < _height; x++)
          if (at(x, y) == OBJECT_BOMB)
            at(x, y) = OBJECT_EMPTY;
    }

    /* Remove bomb instance and create fire */
    if (has_bomb() && _bomb.get()->timer()++ == 2 SEC) {
      _fire = new Fire(_bomb.get()->x(), _bomb.get()->y());
      delete _bomb.get();
    }
  }

  bool burn(i16 x, i16 y) {
    if (x < 0 || x >= _width || y < 0 || y >= _height)
      return false;

    /* Player and fire at the same position */
    if (x == _player->x() && y == _player->y()) {
      _state = GAMEOVER;
      return true;
    }

    /* Break block */
    if (at(x, y) == OBJECT_BLOCK)
      at(x, y) = OBJECT_EMPTY;

    /* Burn everything other than wall and flag */
    return at(x, y) != OBJECT_WALL && at(x, y) != OBJECT_FLAG;
  }

  void draw() {
    clear();

    /* Draw stage */
    for (i16 y = 0; y < _height; y++) {
      for (i16 x = 0; x < _width; x++) {
        switch (at(x, y)) {
          case OBJECT_WALL : mvaddch(y, x, '#'); break;
          case OBJECT_BLOCK: mvaddch(y, x, '@'); break;
          case OBJECT_BOMB : mvaddch(y, x, is_exploding() ? '*' : 'B'); break;
          case OBJECT_FLAG : mvaddch(y, x, 'F'); break;
          default          : mvaddch(y, x, ' ');
        }
      }
    }

    /* Draw player */
    mvaddch(_player->y(), _player->x(), 'P');

    /* Draw fire */
    if (_fire) {
      i16 x = _fire->x(), y = _fire->y();
      if (_fire->timer() < 0.5 SEC) {
        if (burn(x, y)) mvaddch(y, x, '*');
      } else if (_fire->timer() < 1 SEC) {
        if (burn(x, y-1)) mvaddch(y-1, x, '*');
        if (burn(x, y+1)) mvaddch(y+1, x, '*');
        if (burn(x-1, y)) mvaddch(y, x-1, '*');
        if (burn(x+1, y)) mvaddch(y, x+1, '*');
      }
    }

    /* Gameover screen */
    if (is_gameover()) {
      for (i16 y = 0; y < _height; y++)
        for (i16 x = 0; x < _width; x++)
          mvaddch(y, x, '.');
      if (_state == GOT_FLAG) {
        // Find me on remote server :)
        mvprintw(5, 2, "CONGRATZ!");
        mvprintw(7, 1, "SECCON{****");
        mvprintw(8, 1, "***********");
        mvprintw(9, 1, "**********}");
        
      } else {
        mvprintw(5, 2, "GAME OVER");
        mvprintw(7, 1, "PRESS ENTER");
      }
    }
    refresh();
  }

private:
  i16 _width, _height;
  u8 *_field;
  Player *_player;
  Fire *_fire;
  std::unique_ptr<Bomb> _bomb;
  enum GAME_STATE _state;
};

int main() {
  Stage *stage = new Stage(13, 13);

  while (!stage->is_gameover()) {
    stage->tick();
    int ch = getch();
    switch (ch) {
      case KEY_LEFT : stage->move(LEFT);  break;
      case KEY_RIGHT: stage->move(RIGHT); break;
      case KEY_UP   : stage->move(UP);    break;
      case KEY_DOWN : stage->move(DOWN);  break;
      case ' '      :
        if (stage->has_bomb())
          stage->pickup_bomb();
        else
          stage->put_bomb();
        break;
    }
    stage->draw();
  }

  while (getch() != '\n');
  delete stage;
  return 0;
}
