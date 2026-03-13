struct cmd_struct {
  void        *next;
  const char **name;
  int (*fn)(int, const char **);
};

extern struct cmd_struct *commands;
