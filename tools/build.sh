

mkdir /test/aucont/bin
# chmod 777 bin
g++ --std=c++11 /test/aucont/src/aucont_start.cpp /test/aucont/src/util.h -o /test/aucont/bin/aucont_start
g++ --std=c++11 /test/aucont/src/aucont_stop.cpp /test/aucont/src/util.h -o /test/aucont/bin/aucont_stop
g++ --std=c++11 /test/aucont/src/aucont_exec.cpp /test/aucont/src/util.h -o /test/aucont/bin/aucont_exec
g++ --std=c++11 /test/aucont/src/aucont_list.cpp /test/aucont/src/util.h -o /test/aucont/bin/aucont_list
