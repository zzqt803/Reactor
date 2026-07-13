CXX := g++
CXXFLAGS := -Wall -g
BIN_DIR := bin

# 目标可执行文件
CLIENT_TARGET := $(BIN_DIR)/client
SERVER_TARGET := $(BIN_DIR)/server
# 源码文件
CLIENT_SRC := client.cpp
SERVER_SRC := server.cpp

# 伪目标，不会生成同名文件
.PHONY: all clean

# 默认目标，一次性编译两个程序
all: $(BIN_DIR) $(CLIENT_TARGET) $(SERVER_TARGET)

# 创建bin目录（不存在才创建）
$(BIN_DIR):
	mkdir -p $@

# 编译client
$(CLIENT_TARGET): $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) $< -o $@

# 编译server
$(SERVER_TARGET): $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) $< -o $@

# 清理：删除bin文件夹
clean:
	rm -rf $(BIN_DIR)