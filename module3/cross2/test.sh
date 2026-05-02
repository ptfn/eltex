#!/bin/bash
cd /home/ptfn/eltex/module3/cross2

# Запускаем taxi в фоне
./taxi > /tmp/taxi_output.txt 2>&1 &
TAXI_PID=$!

# Ждём startup
sleep 1

# Создаём driver
echo "create_driver"
sleep 0.5

# Читаем PID из сокета
DRIVER_PID=$(ls /tmp/taxi_driver_*.sock 2>/dev/null | sed 's/.*_//' | sed 's/.sock//' | head -1)
echo "Created driver with PID: $DRIVER_PID"

# Проверяем статус
echo "get_drivers"
sleep 0.5

# Отправляем задачу на 3 секунды
echo "send_task $DRIVER_PID 3"
sleep 0.5

# Проверяем статус (должен быть Busy)
echo "get_status $DRIVER_PID"
sleep 1

# Ждём пока задача выполнится
sleep 2

# Проверяем статус (должен быть Available)
echo "get_status $DRIVER_PID"

# Выходим
echo "exit"
sleep 0.5

# Показываем вывод
cat /tmp/taxi_output.txt