
build: so_stdio.dll

so_stdio.dll: libso_stdio.obj 
	link /nologo /dll /out:libso_stdio.dll /implib:so_stdio.lib libso_stdio.obj 
	
libso_stdio.obj: libso_stdio.c libso_stdio.h
	cl /c /MD /W1 /nologo libso_stdio.c /Folibso_stdio.obj

	
clean:
	del *.obj *.lib *.dll *exp