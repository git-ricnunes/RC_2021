# RC 2021

Compilar o projeto: 
make

###### Personal Device
Correr o Personal device:
./pd PDIP [-d PDport] [-n ASIP] [-p ASport]
onde: - PDIP é o IP onde o programa está a correr.
	  - PDport é a porta do psersonal device à escuta para comunicação UDP.
	  - ASIP é p hostname do servidor de autenticação.
	  - ASport é a porta à escuta para comunicação UDP do servidor de autenticação.

Exemplos:
./pd 127.0.0.1 -p 57011 -n tejo.tecnico.ulisboa.pt -p 58011
./pd 193.136.128.104 -p 57011 -n sigma01 -p 58011

## Exemplos de comandos:
> reg 71015 password

>exit

###### Personal Device
Correr o Personal device:
./AS [-p ASport] [-v]
onde: - ASport é a porta à escuta para comunicação UDP.
	  - v flag que activa o modo verboso do servidor de autenticação onde é possivel ver a trocas de mensagens entre outras componentes e o servidor AS.
	  

## TODO comunicação entre o AS e o FS.