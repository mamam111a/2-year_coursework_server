Инструкция по запуску сервера на Ubuntu/Linux:

0) Установка Docker (если ещё не установлен)

Выполните в терминале команду:
"sudo apt update && sudo apt install -y apt-transport-https ca-certificates curl software-properties-common && curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg && echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null && sudo apt update && sudo apt install -y docker-ce docker-ce-cli containerd.io docker-compose-plugin && docker --version"

1) Подготовка и запуск сервера

Перейдите в главную директорию, где находится файл README.md.
Откройте терминал в этой директории.
Загрузите Docker-образ и запустите сервер, выполнив команду:
"docker load -i ./images/ubuntu-24.04.tar && docker compose up --build -d"

2) Просмотр логов сервера
"docker logs coursework-server_container"
