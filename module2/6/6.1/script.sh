gcc -c contact.c -o contact.o
ar rc libcontact.a contact.o
gcc -c main.c -o main.o
gcc main.o -L. -lcontact -o phonebook 
