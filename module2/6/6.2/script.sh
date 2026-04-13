gcc -c -fPIC contact.c -o contact.o
gcc -shared -o libcontact.so contact.o
gcc main.c -L. -lcontact -o phonebook
