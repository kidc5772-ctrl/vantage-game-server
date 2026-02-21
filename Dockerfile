# Dockerfile for Vantage Game Server on Render.com
FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    libsdl2-dev \
    libsdl2-net-dev \
    && rm -rf /var/lib/apt/lists/*

# Create app directory
WORKDIR /app

# Copy server code
COPY game_server.cpp .

# Build the server
RUN g++ game_server.cpp -o game_server -lSDL2 -lSDL2_net -lpthread

# Expose UDP port
EXPOSE 7777/udp

# Run the server
CMD ["./game_server"]
