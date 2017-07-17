
all clean install distclean:
	@make -C kernel $@
	@make -C src    $@

