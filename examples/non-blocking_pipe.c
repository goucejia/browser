int flag;
fcntl(fd[0], F_GETFL, flag);
flag = flag | O_NONBLOCK;
fcntl(fd[0], F_SETFL, flag);
