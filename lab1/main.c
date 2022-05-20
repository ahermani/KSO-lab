int do_getprocnr(void) {
	int i;
  	for (i = 0; i < NR_PROCS; ++i) {
    		if ((mproc[i].mp_flags & IN_USE) && (mproc[i].mp_pid == pid))
      		return i;
  	}
  	return ENOENT;
}
