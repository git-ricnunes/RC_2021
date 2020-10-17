# RC 2021 - GRUPO 11

#### Manuel Gomes Carvalho 87237
#### Miguel Gomes Coelho 89509
#### Ricardo José Duarte Nunes 71015
	
## Compilar o projeto: 
make

## Personal Device
### Correr o Personal device:
./pd PDIP [-d PDport] [-n ASIP] [-p ASport]
###### PDIP é o IP onde o programa está a correr.
###### PDport é a porta do psersonal device à escuta para comunicação UDP.
###### ASIP é p hostname do servidor de autenticação.
###### ASport é a porta à escuta para comunicação UDP do servidor de autenticação.

### Exemplos chamada:
>./pd 127.0.0.1 -p 57011 -n tejo.tecnico.ulisboa.pt -p 58011

>./pd 193.136.128.104 -p 57011 -n sigma01 -p 58011

### Exemplos de comandos:
>reg 71015 password

>exit

## Servidor de autenticação
### Correr o Servidor de autenticação
>./AS [-p ASport] [-v]

###### ASport é a porta à escuta para comunicação UDP.
######  flag v que activa o modo verboso do servidor de autenticação onde é possivel ver a trocas de mensagens entre outras componentes e o servidor AS.
### Exemplos chamada:
>./AS

>./AS -v

>./AS -v -p 58111
	  
## TODO 
### ->Comunicação entre o AS e o FS.
### ->Testes com outras componentes.