In order to build correctly the wrapper in Python 3.8.1 we need to put all the wanted includes in OgCore.i (the interface).
Then we need to tell in setup.py where are all the sources.

Warning: We have to make sure that our .cpp files include the corresponding .h file using "" syntax. Same in OgCore.i.

When compilation is done, the builded module is in build/lib.win32-3.8 as a .pyd extension file.

Note: We may have to define the templates explicitely by ourself.

Usage:
1 - Set the OgCore.i file whith the required .h files.
2 - Set the setup.py file with the corresponding .cpp files according to the .h required in step 1.
3 - Build OgCore project in Visual Studio (Rebuild may be necessary sometimes).
4 - Take out the _OgCore.cp38-win32.pyd module inside build/lib.win32-3.8 folder to where you want to code your python scripts.
5 - At the top of the python script:
	import OgCore
	
	OgCore.X(Y)

    Note: X can be the methods, classes, structures, enums, objects or anything that has been wrapped into python code.
    Everything is now accessible by just typing OgCore.Something.
