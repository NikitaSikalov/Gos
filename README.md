# Задача 3.2

## Условие

Предложите алгоритм, который позволит эффективно хранить множественные однотипные данные - диагностические сообщения. Каждое сообщение выглядит как одна из шаблонных строк, в которую подставлено несколько значений. Количество шаблонов фиксированное и не больше 1000.

Необходимо уметь эффективно сериализовать/десериализовать подобные сообщения в файл/из файла.

Пример: В модуле ТАКСИ произошла ошибка доступа водителя ИВАНА (В модуле НАЗВАНИЕ_МОДУЛЯ произошла ошибка НАЗВАНИЕ_ОШИБКИ СУБЪЕКТ_ОШИБКИ)

## Решение

### Идея решения

#### Примечения к пониманию условия задачи

Я не до конца понял условие и поэтому решал более общую задачу, когда заранее шаблоны неизвестны, но мы считаем, что в 
лог-файле много повторяющихся однотипных шаблонных конструкций.

#### Граф токенов

Идея решения строится на том, что весь наш лог-файл токенизируется на отдельные токены, а из них строится граф. 
В нашем графе вершинами будут всевозможные токены, которые встретились в лог-файле. 
Токены –– это отдельные слова, которы разделены пробельными символами.
Эффективность хранения в памяти достигается за счет того, что повторяющиеся токены будут храниться храниться в единственном экземпляре
(с точностью до позиции в шаблоне). 
Помимо этого в графе присутствуют фиктивные вершины: `START` стартовая вершина и `END` концевая вершина.
Они используются для удобства работы алгоритмов обхода графа и получения очередных строчек лога.
Каждая строчка лога в нашем графе начинается из стартовой вершины, а заканчивается в концевой
вершине, а сама строчка лога является путем из стартовой вершины в концевую.

Заметим, что в лог файле важен порядок строк, поэтому в каждой вершине поддерживается очередь соседних вершин.
Таким образом, когда мы выбираем какую вершину обходить следующией, мы просто достаем из очереди соседних вершин следующую вершину, 
которую нужно обойти.
Дополнительно использовалась оптимизация, если в очереди соседних вершин находится несколько подряд идущих одних тех же вершин, то
в очередь кладется не `n` подряд идущих вершин, а пара из указателя и счетчика. При проходе, мы просто декрементируем
счетчик такой пары.
(Например, такая оптимизация будет очень полезна для примера из [examples](./examples/logs.txt)).

Для эффективности хранения подобного графа, все вершины хранятся на shared поинтерах. Также в вершинах не храним сами токены, 
а храним только их id-шники и дополнительно мапинг этих id-шников.

Описание логики работы графа находится в [logs_reader](./logs_reader/logs_graph.h).
Пример их использования можно найти в примерах [bin](./bin) бинарников.

#### Сериализация

Для сериализации я использовал протобуфы. Описание графа в сериализованном виде можно найти в [./proto/graph.proto](./proto/graph.proto).
Для сериализации мы делаем топологическую сортировку вершин нашего графа, а потом в обратном порядке топологической сортировки записываем
вершины в сериализованный список вершин (то есть концевая вершина `END` будет записана первой, а стартовая `START` последней).
Чтобы индентифицировать вершины в сериализованном состоянии, мы добавляем вершинам уникальные id с помощью `reiterpret_cast` указателя вершины к uint64 
(очевидно, что в памяти процесса указатели будут разные для разных вершин). При десериализации нашего графа мы будем доставать вершины из списка 
так, что к моменту обработки очередной сериализованной вершины, все ее соседи уже будут находиться в графе (для этого использовалась
топологическая сортировка при сериализации).

#### Доп замечания

Важно, что решение не загружет полностью весь текст логов в RAM-память, а алгоритм построен таким образом, чтобы уметь работать на потоке.
Тем не менее сам граф поддерживается в RAM, поэтому совсем-совсем большие лог файлы с большим количеством различных паттернов
обработать скорей всего не удасться. 

Также решение не требудет на вход никаких шаблонов, фактически эти шаблоны оптимизировано распределяются по графу.
Вершины, которые входят в шаблон будут скорей всего те, у которых будет большая входящая или исходящая степень. 
Решение работает абсолютно с произвольным тектовым файлом, но чем больше в нем будет шаблонных строк, тем лучше удасться сжать данные.

Бинарный формат хранения протобуфов, позволяет наиболее оптимальным способом сериализовывать данные и записывать их на 
персистетное хранилище с учетом всех оптимизаций, примененных для RAM-модели.

### Сборка

Для сборки необходимо установить [cmake](https://cmake.org/) и [protobuf](https://github.com/protocolbuffers/protobuf).
Сборка выполняется с помощью `cmake`. Пример сборки:

```
mkdir build
cd build 
cmake ../
make
```

После этого в директории сборки должны появиться два исполняемых файла: `serialize` и `deserialize`.

### Запуск

```
# сериализация
./build/serialize examples/logs.txt ./dump 

# десериализация
./build/deserialize ./dump ./logs.txt
```
