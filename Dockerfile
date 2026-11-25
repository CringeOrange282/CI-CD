FROM ubuntu:24.04

# Отключаем интерактивные вопросы при установке пакетов
ENV DEBIAN_FRONTEND=noninteractive

# Ставим компилятор C++23, CMake, Git, Python и curl
RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    git \
    python3 \
    python3-pip \
    python3-venv \
    curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Создаём виртуальное окружение Python и ставим пакеты для fake_server.py
RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"
RUN pip install --no-cache-dir fastapi httpx uvicorn

# Копируем весь проект в контейнер
COPY . .

# Собираем C++ проект (CMake сам скачает gtest, nlohmann_json и httplib)
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && \
    cmake --build build --config Debug --target orchestra

EXPOSE 8080 9000
