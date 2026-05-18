# Задание 2 — procfs модуль

Взяли пример с `file_operations` (старый API для proc). На ядре 7.0.8 `proc_create` принимает `struct proc_ops`, а не `file_operations`. Сборка падает с ошибкой несовместимости типов.

![Ошибка сборки](screen/1.png)

## Адаптация под новое ядро

Заменили `struct file_operations` на `struct proc_ops`. Поля переименованы: `.open` заменен на `.proc_open`, `.release` на `.proc_release`, `.read` на `.proc_read`, `.write` на `.proc_write`.

После исправления модуль собрался без ошибок.

![Успешная сборка](screen/2.png)

## Загрузка и проверка

Загружаем модуль, пишем данные через `/proc/eltex_task2`, читаем обратно. Проверяем сообщения модуля в dmesg.

![Загрузка, запись, чтение, dmesg](screen/3.png)

