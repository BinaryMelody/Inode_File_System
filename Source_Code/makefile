#vpath %.c src
#vpath %.o build
#vpath %.h src
VPATH = build : src

compileCmd = g++ -g -pthread -lpthread -c $< -o build/$@
linkCmd = cd build && g++ -g -pthread -lpthread $^ -o $@

incfile = headers.h
task1 = BDS
task2 = BDC_command
task3 = BDC_random
task4 = IDS
task5 = FS
task6 = FC
task7 = BDS_for_FS

project += $(task1) $(task2) $(task3) $(task4) $(task5) $(task6) $(task7)

all : $(project)

$(task1): $(task1).o 
	$(linkCmd)

$(task1).o: $(task1).c $(incfile)
	$(compileCmd)

$(task2): $(task2).o
	$(linkCmd)

$(task2).o: $(task2).c $(incfile)
	$(compileCmd)

$(task3): $(task3).o
	$(linkCmd)

$(task3).o: $(task3).c $(incfile)
	$(compileCmd)

$(task4): $(task4).o
	$(linkCmd)

$(task4).o: $(task4).c $(incfile)
	$(compileCmd)

$(task5): $(task5).o
	$(linkCmd)

$(task5).o: $(task5).c $(incfile)
	$(compileCmd)

$(task6): $(task6).o
	$(linkCmd)

$(task6).o: $(task6).c $(incfile)
	$(compileCmd)

$(task7): $(task7).o
	$(linkCmd)

$(task7).o: $(task7).c $(incfile)
	$(compileCmd)


clean:
	rm -r ./build/*

