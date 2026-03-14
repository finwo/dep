struct cmd_struct {
  void        *next;
  const char **name;
  const char  *display;
  const char  *description;
  const char  *help_text;
  int (*fn)(int, const char **);
};

extern struct cmd_struct *commands;
