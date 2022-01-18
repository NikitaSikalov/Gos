# Задача 3.2

## Условие

Предложите алгоритм, который позволит эффективно хранить множественные однотипные данные - диагностические сообщения. Каждое сообщение выглядит как одна из шаблонных строк, в которую подставлено несколько значений. Количество шаблонов фиксированное и не больше 1000.

Необходимо уметь эффективно сериализовать/десериализовать подобные сообщения в файл/из файла.

Пример: В модуле ТАКСИ произошла ошибка доступа водителя ИВАНА (В модуле НАЗВАНИЕ_МОДУЛЯ произошла ошибка НАЗВАНИЕ_ОШИБКИ СУБЪЕКТ_ОШИБКИ)

## Решение

## Сборка

Для сборки необходимо установить [cmake](https://cmake.org/) и [protobuf](https://github.com/protocolbuffers/protobuf).
Сборка выполняется с помощью `cmake`. Пример сборки:

```
mkdir build
cd build 
cmake ../
make
```

После этого должны появиться в директории сборки два исполняемых файла: `serialize` и `deserialize`.

## Запуск

```
# сериализация
./build/serialize examples/logs.txt ./dump 

# десериализация
./build/deserialize ./dump ./logs.txt
```