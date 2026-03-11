# POC Kafka C++

Este projeto demonstra como usar Kafka com C++ através da biblioteca librdkafka, incluindo um ambiente Docker completo com Kafka Confluent 8.0.0 usando KRaft e Kafka-UI.

## 🚀 Como usar

### 1. Subir o ambiente Kafka

```bash
docker-compose up -d
```

### 2. Verificar se os serviços estão rodando

```bash
docker-compose ps
```

### 3. Acessar o Kafka-UI

Abra o navegador em: http://localhost:8080

### 4. Restaurar pacotes e compilar o código C++

Restaure o pacote NuGet nativo `librdkafka.vcpkg` (via Visual Studio ou CLI) e compile o projeto. Veja a seção **Pacote NuGet Nativo** abaixo para mais detalhes.

## 📋 Serviços incluídos

### Kafka (Confluent 8.0.0 com KRaft)
- **Porta**: 9092
- **Modo**: KRaft (sem Zookeeper)
- **JMX**: 9101
- **Configuração**: Single-node cluster

### Kafka-UI
- **Porta**: 8080
- **Interface**: Web UI para gerenciar tópicos, mensagens e consumidores
- **Funcionalidades**: 
  - Visualizar tópicos e partições
  - Enviar e consumir mensagens
  - Monitorar consumidores
  - Métricas JMX

## 🔧 Configurações do Kafka

- **Bootstrap Servers**: `localhost:9092`
- **Cluster ID**: `MkU3OEVBNTcwNTJENDM2Qk`
- **Replication Factor**: 1 (adequado para desenvolvimento)
- **Min ISR**: 1

## 📝 Comandos úteis

### Criar um tópico
```bash
docker exec -it kafka kafka-topics --create --topic test-topic --bootstrap-server localhost:9092 --partitions 3 --replication-factor 1
```

### Listar tópicos
```bash
docker exec -it kafka kafka-topics --list --bootstrap-server localhost:9092
```

### Produzir mensagens via console
```bash
docker exec -it kafka kafka-console-producer --topic test-topic --bootstrap-server localhost:9092
```

### Consumir mensagens via console
```bash
docker exec -it kafka kafka-console-consumer --topic test-topic --from-beginning --bootstrap-server localhost:9092
```

### Parar os serviços
```bash
docker-compose down
```

### Parar e remover volumes (limpar dados)
```bash
docker-compose down -v
```

## 🏗️ Estrutura do projeto

- `PocKafkaCpp.cpp` - Código principal com producer e consumer
- `docker-compose.yml` - Configuração do ambiente Kafka
- `README.md` - Este arquivo

## 📦 Pacote NuGet Nativo

Este projeto utiliza o pacote NuGet nativo **librdkafka.vcpkg** (v2.10.1.3) para integração com Kafka em C++. O pacote está configurado no arquivo `packages.config`.

### Restaurar pacotes via Visual Studio

1. Abra a solução `PocKafkaCpp.sln` no Visual Studio.
2. Clique com o botão direito na solução no **Solution Explorer**.
3. Selecione **Restore NuGet Packages**.

### Restaurar pacotes via CLI

No diretório do projeto (`PocKafkaCpp/`), execute:

```bash
nuget restore PocKafkaCpp.sln
```

> **Nota:** É necessário ter o [NuGet CLI](https://www.nuget.org/downloads) instalado e disponível no PATH.

## ⚠️ Requisitos

- Docker e Docker Compose instalados
- Biblioteca librdkafka (instalada automaticamente via pacote NuGet nativo)
- Visual Studio ou compilador C++ compatível
- NuGet CLI (para restauração de pacotes via linha de comando)

## 🐛 Troubleshooting

### Kafka não inicia
- Verifique se a porta 9092 não está em uso
- Certifique-se de que o Docker tem recursos suficientes

### Kafka-UI não conecta
- Aguarde alguns segundos para o Kafka inicializar completamente
- Verifique os logs: `docker-compose logs kafka-ui`

### Problemas de conexão no código C++
- Certifique-se de que o Kafka está rodando: `docker-compose ps`
- Verifique se o tópico existe no Kafka-UI
- Confirme que está usando `localhost:9092` como bootstrap server
