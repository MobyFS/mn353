#ifndef __RESTART_H
#define __RESTART_H

extern void restart(int type, char *param, unsigned long line);

// В опции к компилятору должне быть включен флаг --no_path_in_file_macros
// для того чтобы макрос __FIILE__ не преобразовывался в абсолютный путь
// к файлу, а только в имя файла (для того, чтобы зашивка не зависила от
// расположения исходных текстов на момент компиляции, а зависила только
// от содержимого исходных текстов).
#define FATAL_ERROR() restart(RST_T_SOFT, __FILE__, __LINE__)

#endif /* __RESTART_H */
