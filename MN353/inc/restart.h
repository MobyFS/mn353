#ifndef __RESTART_H
#define __RESTART_H

extern void restart(int type, char *param, unsigned long line);

// � ����� � ����������� ������ ���� ������� ���� --no_path_in_file_macros
// ��� ���� ����� ������ __FIILE__ �� ���������������� � ���������� ����
// � �����, � ������ � ��� ����� (��� ����, ����� ������� �� �������� ��
// ������������ �������� ������� �� ������ ����������, � �������� ������
// �� ����������� �������� �������).
#define FATAL_ERROR() restart(RST_T_SOFT, __FILE__, __LINE__)

#endif /* __RESTART_H */
