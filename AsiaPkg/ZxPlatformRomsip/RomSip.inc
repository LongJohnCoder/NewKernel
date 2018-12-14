
  ifdef HX002EA0_03
  ifdef PCISIG_PLUGFEST_WORKAROUND
    dd  000000000h
    dd  00038FF9ah	 
    dd  00006ABA8h
    dd  000000000h
  else
	dd  000000000h
	dd  00038FF9ah     
	dd  00003FFFCh
	dd  000000000h
  endif
  endif

  ifdef HX002EB0_00
	dd  000000000h
	dd  00038FF82h
	dd  00003FFFCh
	dd  000000000h
  endif  

  ifdef HX002EB0_11
	dd  000000000h
	dd  00038FFDAh
	dd  00003FFFCh
	dd  000000000h
  endif

  ifdef HX002EC0_01
	dd  000000000h
	dd  00038FF8Ah
	dd  00003FFFCh
	dd  000000000h
  endif

  ifdef HX002EC0_10
	dd  000000000h
	dd  00038FFD2h
	dd  00003FFFCh

	dd  000000000h
  endif

  ifdef HX002ED0_02
	dd  000000000h
	dd  00038FF92h
	dd  00003FFFCh
	dd  000000000h
  endif

  ifdef HX002ED0_10
	dd  000000000h
	dd  00038FFD2h
	dd  00003FFFCh
	dd  000000000h
  endif

  ifdef HX002EE0_04
	dd  000000000h
	dd  00038FFA2h
	dd  00003FFFCh
	dd  000000000h
  endif

  ifdef HX002EE0_05
	dd  000000000h
	dd  00038FFAAh
	dd  00003FFFCh
	dd  000000000h
  endif

  ifdef HX002EH0_01
	dd  000000000h
	dd  00038FF8Ah
	dd  00003FFFCh
	dd  000000000h
  endif

  ifdef HX002EK0_03
	dd  000000000h
	dd  00038FF9ah
	dd  00003FFFCh
	dd  000000000h
  endif
  
  ifdef HX002EL0_05
	dd  000000000h
	dd  00038FFAAh
	dd  00003FFFCh
	dd  000000000h
  endif
