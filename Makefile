CFLAGS += -Wextra -Wall
CXXFLAGS += -Wextra -Wall -std=c++20
CPPFLAGS += -Wextra -Wall

include config/ar
include config/nfs

include zmk/make.mk
