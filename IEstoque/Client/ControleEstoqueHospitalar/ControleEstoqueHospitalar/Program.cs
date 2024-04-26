using System;
using System.ComponentModel.DataAnnotations;
using System.Reflection;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading.Tasks;
using MQTTnet;
using MQTTnet.Client;
using MQTTnet.Extensions.ManagedClient;
using MQTTnet.Packets;
using MQTTnet.Protocol;

namespace ControleEstoqueHospitalar
{
    class Program
    {
        const string topicCadastro = "esp8266/cadastro-medicamento";
        const string topicCadastroRetorno = "esp8266/cadastro-medicamento-retorno";
        const string topicSaida = "esp8266/estoque-saida";
        const string topicEstoque = "esp8266/estoque";
        private static List<CadastroMedicamento> medicamentosCadastrados = new List<CadastroMedicamento>();
        static async Task Main(string[] args)
        {
            Console.WriteLine("Pressione qualquer tecla para iniciar o client de controle de estoque.");
            Console.ReadKey();
            string broker = "broker.emqx.io";
            int port = 1883;
            string clientId = Guid.NewGuid().ToString();

            string username = "emqx";
            string password = "public";

            // Create a MQTT client factory
            var factory = new MqttFactory();

            // Create a MQTT client instance
            var mqttClient = factory.CreateMqttClient();

            // Create MQTT client options
            var options = new MqttClientOptionsBuilder()
                .WithTcpServer(broker, port) // MQTT broker address and port
                .WithCredentials(username, password) // Set username and password
                .WithClientId(clientId)
                .WithCleanSession()
                .Build();

            // Connect to MQTT broker
            var connectResult = await mqttClient.ConnectAsync(options);

            if (connectResult.ResultCode == MqttClientConnectResultCode.Success)
            {
                Console.WriteLine("Connected to MQTT broker successfully.");
                Console.WriteLine();

                // Subscribe to a topic
                await mqttClient.SubscribeAsync(topicCadastroRetorno);
                await mqttClient.SubscribeAsync(topicEstoque);
                var intEstoqueAtual = 4;
                // Callback function when a message is received
                mqttClient.ApplicationMessageReceivedAsync += e =>
                {
                    if (e.ApplicationMessage.Topic == topicCadastroRetorno)
                    {
                        HandlerCadastroMedicamento(e);
                    }

                    if (e.ApplicationMessage.Topic == topicEstoque)
                    {
                        HandlerEstoqueSaida(e, mqttClient);
                    }


                    return Task.CompletedTask;
                };

                InitEstoque(mqttClient);

            }
            else
            {
                Console.WriteLine($"Failed to connect to MQTT broker: {connectResult.ResultCode}");
            }

            Console.ReadKey();
            await mqttClient.DisconnectAsync();
        }

        private static void HandlerCadastroMedicamento(MqttApplicationMessageReceivedEventArgs e)
        {
            Console.WriteLine();
            Console.ForegroundColor = ConsoleColor.Green;
            Console.WriteLine($"Retorno Cadastro Recebido: {Encoding.UTF8.GetString(e.ApplicationMessage.PayloadSegment)}");
            Console.WriteLine();
            var cadastroMedicamentoResponse =
                JsonSerializer.Deserialize<CadastroMedicamento>(Encoding.UTF8.GetString(e.ApplicationMessage.PayloadSegment));

            if (cadastroMedicamentoResponse != null)
            {
                medicamentosCadastrados.Add(cadastroMedicamentoResponse);
            }

            PrintEstoqueMedicamentos();
            PrintMedicamentosCadastrados();
            Console.ForegroundColor = ConsoleColor.White;
            Console.WriteLine();
        }


        private static void HandlerEstoqueSaida(MqttApplicationMessageReceivedEventArgs e, IMqttClient mqttClient)
        {
            Console.WriteLine();
            Console.ForegroundColor = ConsoleColor.Green;
            Console.WriteLine($"Retorno Saida Estoque: {Encoding.UTF8.GetString(e.ApplicationMessage.PayloadSegment)}");
            Console.WriteLine();
            var saidaEstoqueRequest = JsonSerializer.Deserialize<SaidaEstoqueRequest>(Encoding.UTF8.GetString(e.ApplicationMessage.PayloadSegment));

            if (saidaEstoqueRequest != null)
            {
                var medicamentoEstoque = medicamentosCadastrados.FirstOrDefault(m => m.IdTag == saidaEstoqueRequest.IdTag);
                if (medicamentoEstoque != null)
                {
                    medicamentosCadastrados.Remove(medicamentoEstoque);
                    var saidaEstoqueResponse = new SaidaEstoqueResponse
                    {
                        Id = medicamentoEstoque.Id,
                        Medicamento = medicamentoEstoque.Medicamento,
                        Estoque = medicamentosCadastrados.Count(m => m.Id == medicamentoEstoque.Id).ToString()
                    };

                    var saidaEstoquePayload = JsonSerializer.Serialize(saidaEstoqueResponse);
                    var message = new MqttApplicationMessageBuilder()
                        .WithTopic(topicSaida)
                        .WithPayload(saidaEstoquePayload)
                        .WithQualityOfServiceLevel(MqttQualityOfServiceLevel.AtLeastOnce)
                        .Build();

                    mqttClient.PublishAsync(message);

                    PrintEstoqueMedicamentos();
                    PrintMedicamentosCadastrados();
                    Console.ForegroundColor = ConsoleColor.White;
                }
            }

            Console.WriteLine();
        }

        private static void PrintEstoqueMedicamentos()
        {
            Console.WriteLine();
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine($"Estoque de medicamentos disponíveis.");
            var medicamentosGroup = medicamentosCadastrados.GroupBy(m => m.Id);
            foreach (var estoque in medicamentosGroup)
            {
                var medicamento = estoque.First().Medicamento;
                var id = estoque.First().Id;
                var quantidade = estoque.Count();

                Console.WriteLine($"ID: {id} - MEDICAMENTO: {medicamento} - QUANTIDADE: {quantidade}");
            }

            Console.ForegroundColor = ConsoleColor.White;
            Console.WriteLine();
        }

        private static void PrintMedicamentosCadastrados()
        {
            Console.WriteLine();
            // Define as cores das fontes
            Console.ForegroundColor = ConsoleColor.Yellow;

            Console.WriteLine($"Medicamentos Cadastrados.");

            // Obter propriedades com atributo MaxLength
            PropertyInfo[] properties = typeof(CadastroMedicamento).GetProperties();
            Dictionary<string, int> maxLengths = new Dictionary<string, int>();
            foreach (var property in properties)
            {
                var maxLengthAttribute = property.GetCustomAttribute(typeof(MaxLengthAttribute)) as MaxLengthAttribute;
                if (maxLengthAttribute != null)
                {
                    maxLengths[property.Name] = maxLengthAttribute.Length;
                }
            }

            // Cabeçalho da tabela
            Console.WriteLine($"|{"ID",42}|{"MEDICAMENTO",42}|{"ID TAG",11}|");

            // Linha de separação
            Console.WriteLine(new string('-', maxLengths["Id"] + maxLengths["Medicamento"] + maxLengths["IdTag"] + 9));

            // Conteúdo da tabela
            foreach (var medicamento in medicamentosCadastrados)
            {
                Console.WriteLine($"|{medicamento.Id,42}|{medicamento.Medicamento,42}|{medicamento.IdTag,11}|");
            }

            // Restaura a cor padrão da fonte
            Console.ForegroundColor = ConsoleColor.White;
            Console.WriteLine();
        }

        private static void InitEstoque(IMqttClient mqttClient)
        {
            var id = Guid.NewGuid().ToString();
            for (int i = 0; i < 4; i++)
            {
                Console.ForegroundColor = ConsoleColor.Blue;
                Console.WriteLine($"Cadastrando medicamento: {i + 1}");
                var medicamento = new CadastroMedicamento
                {
                    Id = id,
                    Medicamento = "Dexilant 60mg"
                };

                var medicamentoCadastroPayload = JsonSerializer.Serialize(medicamento);

                var message = new MqttApplicationMessageBuilder()
                    .WithTopic(topicCadastro)
                    .WithPayload(medicamentoCadastroPayload)
                    .WithQualityOfServiceLevel(MqttQualityOfServiceLevel.AtLeastOnce)
                    .Build();

                mqttClient.PublishAsync(message);
                Thread.Sleep(10000);
            }

            Console.ForegroundColor = ConsoleColor.White;
            Console.WriteLine();
        }

        public class CadastroMedicamento
        {
            [MaxLength(40)]
            [JsonPropertyName("id")]
            public string Id { get; set; }

            [MaxLength(40)]
            [JsonPropertyName("medicamento")]
            public string Medicamento { get; set; }

            [MaxLength(9)]
            [JsonPropertyName("idTag")]
            public string IdTag { get; set; }

        }

        public class SaidaEstoqueRequest
        {
            [JsonPropertyName("idTag")]
            public string IdTag { get; set; }
        }

        public class SaidaEstoqueResponse
        {
            [JsonPropertyName("medicamento")]
            public string Medicamento { get; set; }
            [JsonPropertyName("id")]
            public string Id { get; set; }

            [JsonPropertyName("estoque")]
            public string Estoque { get; set; }
        }
    }
}
