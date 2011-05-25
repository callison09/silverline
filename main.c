/*
Copyright (c) 2011 Christopher Allison

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#include <ncurses.h>
#include <curl/curl.h>
#include <string.h>
#include <assert.h>
#include <zlib.h>


#define SET_BINARY_MODE(file)
#define CHUNK 524288
/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
int inf(FILE *source, FILE *dest)
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit2(&strm, 14);
    if (ret != Z_OK)
        return ret;
    /* decompress until deflate stream ends or end of file */
    do {
      strm.avail_in = fread(in, 1, CHUNK, source);
      if (ferror(source)) {
          (void)inflateEnd(&strm);
          return Z_ERRNO;
      }
      if (strm.avail_in == 0)
          break;
      strm.next_in = in;
      /* run inflate() on input until output buffer not full */
      do {
        strm.avail_out = CHUNK;
        strm.next_out = out;
        ret = inflate(&strm, Z_NO_FLUSH);
        assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
        switch (ret) {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR;     /* and fall through */
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            (void)inflateEnd(&strm);
            return ret;
        }
      have = CHUNK - strm.avail_out;
      if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
        (void)inflateEnd(&strm);
        return Z_ERRNO;
      }
      } while (strm.avail_out == 0);
    /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);
    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

void zerr(int ret)
{
    fputs("zpipe: ", stderr);

    switch (ret) {
    case Z_ERRNO:
        if (ferror(stdin))
            fputs("error reading stdin\n", stderr);
        if (ferror(stdout))
            fputs("error writing stdout\n", stderr);
        break;
    case Z_STREAM_ERROR:
        fputs("invalid compression level\n", stderr);
        break;
    case Z_DATA_ERROR:
        fputs("invalid or incomplete deflate data\n", stderr);
        break;
    case Z_MEM_ERROR:
        fputs("out of memory\n", stderr);
        break;
    case Z_VERSION_ERROR:
        fputs("zlib version mismatch!\n", stderr);
    default:
        fputs("No error listed", stderr);
    }
}

int main_menu();
int status_bar(char* status, int s);


int avg_menu();
int avg_control(int control, WINDOW *win);
int install_avg();

void print_menu(WINDOW *menu_win,
                int highlight,
                char **choices,
                int n_choices,
                char *title);

void get_page( const char* url,
              const char* file_name);

int progress_callback(void *clientp,
                        double dltotal,
                        double dlnow,
                        double ultotal,
                        double ulnow);

void init_color_pairs();
void dwin(WINDOW *local_win);

int main(/*int argc, char *argv[]*/) {
  
  int choice, avg_choice;
  int exit = 0;
  int loop = 0;
  initscr();
  clear();
  noecho();
  cbreak();
  curs_set(0);

  start_color();
  init_color_pairs();
  wbkgd(stdscr, COLOR_PAIR(4));
  status_bar("WELCOME TO SILVERLINE - Use the arrow keys to navigate and [TAB] or [ENTER]\n to select menu items", 1);
  while(exit == 0)
  {
    refresh();
    choice = main_menu();
    refresh();
    switch(choice)
    {
      case 1:
        avg_choice = avg_menu();
        break;
      case 5:
        exit = 1;
        break;
      default:
        break;
    }
    loop++;
  }
  clrtoeol();
  refresh();
  endwin();

  return 0;
}

int main_menu(){
  char *main_choices[] ={
  "Utility",
  "Anti Virus",
  "Back Up",
  "Settings",
  "Exit",
  };
  
  WINDOW *main_menu_win;
  int highlight = 1;
  int choice = 0;
  int c, maxx, maxy;
  int n_choices = sizeof(main_choices) / sizeof(char *);
  getmaxyx(stdscr, maxy, maxx);
  int startx = 0;
  int starty = 0;
  int w, h;
  char title[] = "Main Menu";
  startx = (maxx - maxx) + 1;
  starty = (maxy - maxy) + 1;
  h = maxy - 12;
  w = maxx / 3;
  main_menu_win = newwin(h, w, starty, startx);
  keypad(main_menu_win, TRUE);
  
  print_menu(main_menu_win, highlight, main_choices, n_choices, title);
  while(1)
  {
    c = wgetch(main_menu_win);
    switch(c)
    {
      case KEY_UP:
        if(highlight == 1)
          highlight = n_choices;
        else
          --highlight;
        break;
      case KEY_DOWN:
        if (highlight == n_choices)
          highlight = 1;
        else
          ++highlight;
        break;
      case 10:
        choice = highlight;
        break;
      case 9:
        choice = highlight;
        break;
      default:
        status_bar("Invalid character", 2);
        refresh();
        break;
    }
    print_menu(main_menu_win, highlight, main_choices, n_choices, title);
    if(choice != 0)
      break;
  }
  
  return choice;
}

int avg_menu(){
  char *avg_choices[] = {
          "Download",
          "Install",
          "Return",
          };
  
  WINDOW *avg_menu_win;
  int highlight = 1;
  int choice = 0;
  int c, h, w, maxx, maxy;
  getmaxyx(stdscr, maxy, maxx);
  int n_choices = sizeof(avg_choices) / sizeof(char *);
  int startx = 0;
  int starty = 0;
  char title[] = "AVG Menu";
  startx = maxx / 3 + 1;
  starty = (maxy - maxy) + 1;
  h = maxy - 12;
  w = maxx / 3;
  avg_menu_win = newwin(h, w, starty, startx);
  keypad(avg_menu_win, TRUE);
  
  print_menu(avg_menu_win, highlight, avg_choices, n_choices, title);
  while(1)
  {
    c = wgetch(avg_menu_win);
    switch(c)
    {
      case KEY_UP:
        if(highlight == 1)
          highlight = n_choices;
        else
          --highlight;
        break;
      case KEY_DOWN:
        if (highlight == n_choices)
          highlight = 1;
        else
          ++highlight;
        break;
      case 10:
        choice = highlight;
        break;
      case 9:
        choice = highlight;
        break;
      default:
        status_bar("Invalid character", 2);
        refresh();
        break;
    }
    print_menu(avg_menu_win, highlight, avg_choices, n_choices, title);
    if(choice != 0)
      break;
  }

  avg_control(choice, avg_menu_win);
  
  return 0;
}

int avg_control(int control, WINDOW *win)
{
  switch(control)
  {
    case 1:
      get_page("aa-download.avg.com/filedir/inst/avg85flx-r855-a3656.i386.tar.gz",
               "/tmp/avg.tar.gz");
      break;
    case 2:
      install_avg();
    case 3:
      dwin(win);
      refresh();
      break;
    default:
      break;
  }
  return 0;
}
int install_avg()
{
  FILE *dltest;
  FILE *decom;
  SET_BINARY_MODE(dltest);
  SET_BINARY_MODE(dcom);
  int ret;
  decom = fopen("/tmp/avg.tar", "w");
  if((dltest = fopen("/tmp/avg.tar.gz", "w")) != NULL )
  {
    status_bar("Decompressing...", 1);
    ret = inf(dltest, decom);
    if (ret != Z_OK)
      zerr(ret);
  }
  else
    return 0;  
  return 0;
}
void print_menu(WINDOW *menu_win, int highlight, char **choices, int n_choices, char *title)
{
  int x, y, i;
  x = 2;
  y = 3;
  box(menu_win, 0, 0);
  wbkgd(menu_win, COLOR_PAIR(5));
  wattron(menu_win, A_BOLD);
  mvwprintw(menu_win, y-2, x,"%s", title);
  wattroff(menu_win, A_BOLD);
  for(i = 0; i < n_choices; ++i)
  {
    if(highlight == i + 1)
    {
      wattron(menu_win, COLOR_PAIR(6));
      mvwprintw(menu_win, y, x, ">%s", choices[i]);
      wattroff(menu_win, COLOR_PAIR(6));
    }
    else
      mvwprintw(menu_win, y, x, " %s  ", choices[i]);
    ++y;
  }
  wrefresh(menu_win);
}

void get_page( const char* url, const char* file_name)
{
  int success;
  curl_global_init(CURL_GLOBAL_ALL);
  
  CURL* easyhandle;
  easyhandle = curl_easy_init();
  curl_easy_setopt(easyhandle, CURLOPT_URL, url);
  FILE* opn = fopen( file_name, "w");
  curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, opn);
  curl_easy_setopt(easyhandle, CURLOPT_NOPROGRESS, false);
  curl_easy_setopt(easyhandle, CURLOPT_PROGRESSFUNCTION, progress_callback);
  success = curl_easy_perform(easyhandle);
  curl_easy_cleanup(easyhandle);
  if(success != 0)
    mvprintw(23, 0, "Error code: %d", success);
  fclose(opn);
}

int progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
  WINDOW *dlmeter;
  
  int maxy, maxx;
  getmaxyx(stdscr, maxy, maxx);
  int starty = maxy - 6;
  int startx = maxx - maxx;
  int total=40;
  double downloaded=dlnow / dltotal;
  double downloadedMiB=dlnow / dltotal * 100;
  double full=downloaded * total;
  double dlMiB = dlnow/1024/1024;
  double dlTMiB = dltotal/1024/1024;
  int ii=0;
  
  dlmeter = newwin(3, 44, starty, startx);
  wbkgd(dlmeter, COLOR_PAIR(4));
  box(dlmeter, 0, 0);
  mvwprintw(dlmeter, 0, 1, "%3.0f%% ", downloadedMiB);
  wprintw(dlmeter, "Complete: %4.2fMiB  Total: %4.2fMiB", dlMiB, dlTMiB);
  wmove(dlmeter, 1, 1);
  wprintw(dlmeter, " ");
  for( ; ii < full; ii++)
  {
    wattron(dlmeter, COLOR_PAIR(2));
    wprintw(dlmeter, " ");
    wattroff(dlmeter, COLOR_PAIR(2));
  }
  for( ; ii < total; ii++)
  {
    wattron(dlmeter, COLOR_PAIR(3));
    wprintw(dlmeter, "-");
    wattroff(dlmeter, COLOR_PAIR(3));
  }
  wprintw(dlmeter, " ");
  if(dlnow==dltotal)
    mvwprintw(dlmeter, 4, 2, "Done!");
  wrefresh(dlmeter);
  refresh();
  return 0;
}

void init_color_pairs()
{
  init_pair(1, COLOR_RED, COLOR_BLUE);
  init_pair(2, COLOR_BLACK, COLOR_GREEN);
  init_pair(3, COLOR_BLACK, COLOR_WHITE);
  init_pair(4, COLOR_WHITE, COLOR_BLUE);
  init_pair(5, COLOR_BLACK, COLOR_WHITE);
  init_pair(6, COLOR_RED, COLOR_WHITE);
  init_pair(7, COLOR_GREEN, COLOR_BLUE);
}

int status_bar(char* status, int s)
{
  WINDOW *status_win;
  int maxy, maxx;
  int starty, startx;
  getmaxyx(stdscr, maxy, maxx);
  int h;
  h = 4;  
  startx = 0;
  starty = maxy - h - 1;
  status_win = newwin(h, maxx, starty, startx);
  wbkgd(status_win, COLOR_PAIR(4));
  
  switch(s)
  {
    case 1: //Normal
      wattron(status_win, COLOR_PAIR(4) | A_BOLD);
      mvwprintw(status_win, 1, 1, "%s", status);
      wattroff(status_win, COLOR_PAIR(4) | A_BOLD);
      break;
    case 2: //Error
      wattron(status_win, COLOR_PAIR(1) | A_BOLD);
      mvwprintw(status_win, 1, 1, "%s", status);
      wattroff(status_win, COLOR_PAIR(1) | A_BOLD);
      break;
    case 3: //Warning
      wattron(status_win, COLOR_PAIR(7) | A_BOLD);
      mvwprintw(status_win, 1, 1, "%s", status);
      wattroff(status_win, COLOR_PAIR(7) | A_BOLD);
      break;
    default:
      wattron(status_win, COLOR_PAIR(4) | A_BOLD);
      mvwprintw(status_win, 1, 1, "%s", status);
      wattroff(status_win, COLOR_PAIR(4) | A_BOLD);
      break;
  }
  box(status_win, 0, 0);
  wrefresh(status_win);
  return 0;
}

void dwin(WINDOW *local_win)
{	
	wborder(local_win, ' ', ' ', ' ',' ',' ',' ',' ',' ');
        wbkgd(local_win, COLOR_PAIR(4));
        wrefresh(local_win);
        werase(local_win);
	wrefresh(local_win);
	delwin(local_win);
}