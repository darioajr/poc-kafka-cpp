// PocKafkaCpp.cpp : Este arquivo contém a função 'main'. A execução do programa começa e termina ali.
//

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>
#include <csignal>
#include <librdkafka/rdkafkacpp.h>

// Configurações globais
static bool run = true;

static void sigterm(int sig) {
    run = false;
}

// Callback para relatórios de entrega do Producer
class DeliveryReportCb : public RdKafka::DeliveryReportCb {
public:
    void dr_cb(RdKafka::Message &message) override {
        if (message.err()) {
            std::cerr << "Message delivery failed: " << message.errstr() << std::endl;
        } else {
            std::cout << "Message delivered to topic " << message.topic_name() 
                      << " [" << message.partition() << "] at offset " 
                      << message.offset() << std::endl;
        }
    }
};

// Callback para eventos
class EventCb : public RdKafka::EventCb {
public:
    void event_cb(RdKafka::Event &event) override {
        switch (event.type()) {
            case RdKafka::Event::EVENT_ERROR:
                if (event.err() == RdKafka::ERR__ALL_BROKERS_DOWN) {
                    std::cerr << "All brokers are down!" << std::endl;
                    run = false;
                }
                std::cerr << "ERROR (" << RdKafka::err2str(event.err()) << "): " 
                          << event.str() << std::endl;
                break;
            case RdKafka::Event::EVENT_LOG:
                std::cout << "LOG-" << event.severity() << "-" << event.fac() 
                          << ": " << event.str() << std::endl;
                break;
            default:
                std::cout << "EVENT " << event.type() << " (" 
                          << RdKafka::err2str(event.err()) << "): " 
                          << event.str() << std::endl;
                break;
        }
    }
};

// Callback para rebalanceamento do Consumer
class RebalanceCb : public RdKafka::RebalanceCb {
public:
    void rebalance_cb(RdKafka::KafkaConsumer *consumer,
                      RdKafka::ErrorCode err,
                      std::vector<RdKafka::TopicPartition*> &partitions) override {
        std::cout << "Rebalance: ";
        
        if (err == RdKafka::ERR__ASSIGN_PARTITIONS) {
            std::cout << "assigning " << partitions.size() << " partition(s)" << std::endl;
            consumer->assign(partitions);
        } else if (err == RdKafka::ERR__REVOKE_PARTITIONS) {
            std::cout << "revoking " << partitions.size() << " partition(s)" << std::endl;
            consumer->unassign();
        } else {
            std::cerr << "failed: " << RdKafka::err2str(err) << std::endl;
            consumer->unassign();
        }
    }
};

// Função para criar configuração do Producer
std::unique_ptr<RdKafka::Conf> createProducerConfig(const std::string& brokers) {
    std::unique_ptr<RdKafka::Conf> conf(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
    std::string errstr;
    
    if (conf->set("bootstrap.servers", brokers, errstr) != RdKafka::Conf::CONF_OK) {
        std::cerr << "Failed to set bootstrap.servers: " << errstr << std::endl;
        return nullptr;
    }
    
    if (conf->set("acks", "all", errstr) != RdKafka::Conf::CONF_OK) {
        std::cerr << "Failed to set acks: " << errstr << std::endl;
        return nullptr;
    }
    
    return conf;
}

// Função para criar configuração do Consumer
std::unique_ptr<RdKafka::Conf> createConsumerConfig(const std::string& brokers, const std::string& groupId) {
    std::unique_ptr<RdKafka::Conf> conf(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
    std::string errstr;
    
    if (conf->set("bootstrap.servers", brokers, errstr) != RdKafka::Conf::CONF_OK) {
        std::cerr << "Failed to set bootstrap.servers: " << errstr << std::endl;
        return nullptr;
    }
    
    if (conf->set("group.id", groupId, errstr) != RdKafka::Conf::CONF_OK) {
        std::cerr << "Failed to set group.id: " << errstr << std::endl;
        return nullptr;
    }
    
    if (conf->set("auto.offset.reset", "earliest", errstr) != RdKafka::Conf::CONF_OK) {
        std::cerr << "Failed to set auto.offset.reset: " << errstr << std::endl;
        return nullptr;
    }
    
    return conf;
}

// Função do Producer
int runProducer(const std::string& brokers, const std::string& topic) {
    std::cout << "\n=== Kafka Producer ===" << std::endl;
    std::cout << "Brokers: " << brokers << std::endl;
    std::cout << "Topic: " << topic << std::endl;
    
    auto conf = createProducerConfig(brokers);
    if (!conf) {
        return 1;
    }
    
    DeliveryReportCb dr_cb;
    EventCb event_cb;
    
    std::string errstr;
    if (conf->set("dr_cb", &dr_cb, errstr) != RdKafka::Conf::CONF_OK) {
        std::cerr << "Failed to set delivery report callback: " << errstr << std::endl;
        return 1;
    }
    
    if (conf->set("event_cb", &event_cb, errstr) != RdKafka::Conf::CONF_OK) {
        std::cerr << "Failed to set event callback: " << errstr << std::endl;
        return 1;
    }
    
    std::unique_ptr<RdKafka::Producer> producer(RdKafka::Producer::create(conf.get(), errstr));
    if (!producer) {
        std::cerr << "Failed to create producer: " << errstr << std::endl;
        return 1;
    }
    
    std::cout << "Producer created successfully" << std::endl;
    
    // Enviar mensagens de teste
    std::vector<std::string> messages = {
        "Hello Kafka from C++!",
        "This is message number 2",
        "Testing Kafka producer functionality",
        "Message with timestamp",
        "Final test message"
    };
    
    for (size_t i = 0; i < messages.size(); ++i) {
        std::string message = "Message #" + std::to_string(i + 1) + ": " + messages[i];
        
        RdKafka::ErrorCode resp = producer->produce(
            topic,
            RdKafka::Topic::PARTITION_UA,
            RdKafka::Producer::RK_MSG_COPY,
            const_cast<char*>(message.c_str()),
            message.size(),
            nullptr,
            0,
            0,
            nullptr
        );
        
        if (resp != RdKafka::ERR_NO_ERROR) {
            std::cerr << "Failed to produce message: " << RdKafka::err2str(resp) << std::endl;
        } else {
            std::cout << "Message queued: " << message << std::endl;
        }
        
        producer->poll(0);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "Flushing final messages..." << std::endl;
    producer->flush(10000);
    std::cout << "Producer finished successfully" << std::endl;
    
    return 0;
}

// Função do Consumer
int runConsumer(const std::string& brokers, const std::string& topic, const std::string& groupId, int maxMessages) {
    std::cout << "\n=== Kafka Consumer ===" << std::endl;
    std::cout << "Brokers: " << brokers << std::endl;
    std::cout << "Topic: " << topic << std::endl;
    std::cout << "Group ID: " << groupId << std::endl;
    std::cout << "Max Messages: " << maxMessages << std::endl;
    
    auto conf = createConsumerConfig(brokers, groupId);
    if (!conf) {
        return 1;
    }
    
    EventCb event_cb;
    RebalanceCb rebalance_cb;
    
    std::string errstr;
    if (conf->set("event_cb", &event_cb, errstr) != RdKafka::Conf::CONF_OK) {
        std::cerr << "Failed to set event callback: " << errstr << std::endl;
        return 1;
    }
    
    if (conf->set("rebalance_cb", &rebalance_cb, errstr) != RdKafka::Conf::CONF_OK) {
        std::cerr << "Failed to set rebalance callback: " << errstr << std::endl;
        return 1;
    }
    
    std::unique_ptr<RdKafka::KafkaConsumer> consumer(RdKafka::KafkaConsumer::create(conf.get(), errstr));
    if (!consumer) {
        std::cerr << "Failed to create consumer: " << errstr << std::endl;
        return 1;
    }
    
    std::cout << "Consumer created successfully" << std::endl;
    
    std::vector<std::string> topics = {topic};
    RdKafka::ErrorCode resp = consumer->subscribe(topics);
    if (resp != RdKafka::ERR_NO_ERROR) {
        std::cerr << "Failed to subscribe to topic: " << RdKafka::err2str(resp) << std::endl;
        return 1;
    }
    
    std::cout << "Subscribed to topic: " << topic << std::endl;
    std::cout << "Waiting for messages..." << std::endl;
    
    int message_count = 0;
    int timeout_count = 0;
    const int max_timeouts = 10;
    
    while (message_count < maxMessages && timeout_count < max_timeouts) {
        std::unique_ptr<RdKafka::Message> msg(consumer->consume(1000));
        
        switch (msg->err()) {
            case RdKafka::ERR_NO_ERROR:
                message_count++;
                timeout_count = 0;
                std::cout << "\n--- Message " << message_count << " ---" << std::endl;
                std::cout << "Topic: " << msg->topic_name() << std::endl;
                std::cout << "Partition: " << msg->partition() << std::endl;
                std::cout << "Offset: " << msg->offset() << std::endl;
                
                if (msg->key()) {
                    std::cout << "Key: " << std::string(reinterpret_cast<const char*>(msg->key()), msg->key_len()) << std::endl;
                }
                
                if (msg->payload()) {
                    std::cout << "Payload: " << std::string(static_cast<const char*>(msg->payload()), msg->len()) << std::endl;
                }
                break;
                
            case RdKafka::ERR__TIMED_OUT:
                timeout_count++;
                std::cout << "." << std::flush;
                break;
                
            case RdKafka::ERR__PARTITION_EOF:
                std::cout << "\nReached end of partition" << std::endl;
                break;
                
            default:
                std::cerr << "\nConsume failed: " << msg->errstr() << std::endl;
                break;
        }
    }
    
    std::cout << "\nShutting down consumer..." << std::endl;
    consumer->close();
    std::cout << "Consumer finished. Total messages consumed: " << message_count << std::endl;
    
    return 0;
}

int main()
{
    std::cout << "=== POC Kafka C++ ===" << std::endl;
    std::cout << "Demonstrando Producer e Consumer integrados" << std::endl;
    
    // Configurações do Kafka
    std::string brokers = "localhost:9092";
    std::string topic = "test-topic";
    std::string groupId = "poc-cpp-consumer-group";
    
    std::cout << "\nConfiguração:" << std::endl;
    std::cout << "Brokers: " << brokers << std::endl;
    std::cout << "Topic: " << topic << std::endl;
    std::cout << "Group ID: " << groupId << std::endl;
    
    // Configurar signal handlers
    signal(SIGINT, sigterm);
    signal(SIGTERM, sigterm);
    
    // Executar Producer
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "EXECUTANDO PRODUCER" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    int producerResult = runProducer(brokers, topic);
    if (producerResult != 0) {
        std::cerr << "Producer falhou com código: " << producerResult << std::endl;
        return producerResult;
    }
    
    // Aguardar um pouco para garantir que as mensagens foram enviadas
    std::cout << "\nAguardando 3 segundos antes de iniciar o consumer..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Executar Consumer
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "EXECUTANDO CONSUMER" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    int consumerResult = runConsumer(brokers, topic, groupId, 5);
    if (consumerResult != 0) {
        std::cerr << "Consumer falhou com código: " << consumerResult << std::endl;
        return consumerResult;
    }
    
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "POC KAFKA C++ CONCLUÍDA COM SUCESSO!" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    return 0;
}
