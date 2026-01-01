
# CHAT_em_C

## Descrição

CHAT_em_C é um sistema simples de transferência de arquivos via TCP, desenvolvido em linguagem C. O projeto é composto por dois programas: um cliente e um servidor. O cliente envia um arquivo para o servidor, que o recebe e armazena em uma pasta especificada. O objetivo é demonstrar conceitos de redes, sockets e manipulação de arquivos em C.

## Funcionamento

- **Cliente:** Envia um arquivo para o servidor via TCP.
- **Servidor:** Recebe o arquivo e o salva em uma pasta definida pelo usuário.

## Como Executar

### Compilação

Compile os programas usando o gcc:

```bash
gcc client/client.c -o client/client
gcc server/server.c -o server/server
```

### Execução do Servidor

O servidor deve ser iniciado primeiro. Ele recebe três parâmetros:

1. Pasta onde os arquivos serão salvos
2. IP para escutar
3. Porta para escutar

Exemplo:

```bash
./server/server [PASTA] [IP] [PORTA]
```

Exemplo prático:

```bash
./server/server arquivos 127.0.0.1 8080
```

### Execução do Cliente

O cliente também recebe três parâmetros:

1. Caminho do arquivo a ser enviado
2. IP de destino (onde está o servidor)
3. Porta de destino

Exemplo:

```bash
./client/client [ARQUIVO] [IP] [PORTA]
```

Exemplo prático:

```bash
./client/client texto.txt 127.0.0.1 8080
```

## Detalhes Técnicos

- Utiliza sockets TCP (`AF_INET`, `SOCK_STREAM`).
- O cliente envia primeiro o nome do arquivo, depois o conteúdo.
- O servidor aceita múltiplas conexões (até 30 clientes simultâneos).
- O servidor salva os arquivos recebidos na pasta especificada.
- O tamanho do buffer de transmissão é de 1024 bytes.
- O projeto não implementa autenticação ou criptografia.

## Requisitos

- Linux
- GCC
- Permissões para criar pastas e arquivos no diretório do servidor

## Observações

- O projeto é didático e pode ser expandido para incluir chat, autenticação, criptografia, etc.
- Para testes locais, utilize o IP `127.0.0.1` tanto no cliente quanto no servidor.
