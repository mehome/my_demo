
#define MAX 64

#define FAVOR_STATION_NUM 6

enum {
    INIT_LCD,
    CANCEL_PID,
    CLEAR_LCD,
    STANDBY_MODE,
    SHOW_PLAY_STATION,
    PAUSE_PLAYER,
    FAVORITE_STATION,
    PLAYER_MODE,
    PLAYER_VOLUME,
};

struct usr_args {
    int  cmd;
    char station[MAX];
    int  mode;
    char dev[MAX];
    char favor_st[FAVOR_STATION_NUM][MAX];
    int  volume;
};

enum {
    ST7565P_LCD_INIT,
    ST7565P_LCD_CLEAR,
    ST7565P_LCD_WRITE_PAGE,
    ST7565P_LCD_WRITE_PAGE_COL,
    ST7565P_LCD_LSHIFT_COL_OF_PAGE,
};

struct lcd_args {
    char          string[MAX];
    unsigned char byte[132];
    int           page;
    int           s_col;
    int           e_col;
};
