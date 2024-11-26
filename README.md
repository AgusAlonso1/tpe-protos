# tpe-protos
Servidor de POP3 con servicio de management, implementado por Valentina Marti Reta, Tomas Becerra y Agustin Alonso.

## Compilación
Para compilar el proyecto ejecutar el siguiente comando en la base del repositorio.
~~~
make clean all
~~~
Esto generara dos ejecutables: serverx y mng. serverx es el ejecutable propio del servidor, asi como mng es la aplicación cliente del protocolo de management.

## Uso
Ejecutar los siguientes comandos en la base del repositorio para consultar los parametros que reciben los dos binarios.
Para el servidor:
~~~
./serverx -h
~~~
Para el protocolo de management:
~~~
./mng -h
~~~

Este mismo utiliza un token de autorización es cual fue definido arbitrariamente de la siguiente manera: "aaaaaaaa".