
# Máquina escalonadora

Trabalho sobre camada de aplicação, matéria de Redes de Computadores. Esse trabalho envolve conceitos de threads e pacotes TCP.

## Como compilar

**Passo 1.** Abra o terminal na pasta com o _Makefile_.

**Passo 2.** Use o seguinte comando

```
make all
```

Três arquivos deverão aparecer na pasta: _client.out_, _server.out_ e _portal.out_.

## Como executar

Para executar e testar, é necessário inicializar os servidores, o portal e só depois o cliente. Segue as instruções de como executar cada um.

### server.out (servidores)

O único argumento necessário para executar o servidor é a **porta** em que ele ouvirá.

```
./server.out [porta]
```

Como são três servidores, será necessário executar o comando três vezes com portas diferentes.

Existem três comandos pré-programados que executam o servidor em portas 8081, 8082 e 8083, respectivamente.

```
make serv1
```
```
make serv2
```
```
make serv3
```

### portal.out (portal)

O portal precisa de 8 argumentos: a **porta** em que ouvirá, se é **round-robin** _(0 se falso, 1 se verdadeiro)_, o **ip** do primeiro servidor, a **porta** do primeiro servidor, o **ip** do segundo servidor, a **porta** do segundo servidor, o **ip** do terceiro servidor e a **porta** do terceiro servidor.

```
./portal.out (porta) (round-robin) (ip-server1) (porta-server1) (ip-server2) (porta-server2) (ip-server3) (porta-server3) 
```

Existe um comando que executa o portal na **porta 8080**, com o **round-robin** verdadeiro e os servidores com os **ips 127.0.0.1** e portas **8081, 8082 e 8083**.

```
make port
```

### client.out (cliente)

O cliente leva como argumentos o *ip* e a *porta* do portal.

```
./client.out (ip-portal) (porta-portal)
```

Existe um comando que executa o cliente com os parâmetros **127.0.0.1** no ip e **8080** na porta.

```
make cli
```

## COMO USAR

No terminal do cliente, existem três comandos disponíveis: **S** **L** e **Q**

### S

Exemplo de uso:

```
S file1.c file2.c filen.c
```

Envia e retorna o resultado da compilação dos arquivos ```file1.c file2.c filen.c```

Os arquivos deverão estar com o nome correto, no diretório de compilação e separados por um espaço simples.

### L

Exemplo de uso:

```
L
```

Mostra na tela todos os arquivos *.c presentes no diretório de compilação.

### Q

Exemplo de uso:

```
Q
```

Encerra a execução.


