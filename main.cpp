#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
//Reportar erros
#include <stdexcept>
//EXIT_SUCESS e EXIT_FAILURE
#include <cstdlib>
#include <vector>
#include <map>

//Variaveis pra janela
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

//Criando as layers de validacao que vamos usar, todas as validacoes padroes sao agrupadas na layer chamada VK_LAYER_KHRONOS_validation
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

//Se estiver no modo Release desativa as camadas de validacao, esses macros sao parte do C++
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// Armazena os objetos Vulkan como membros privados da classe e cria funcoes que inicializa eles
class HelloTriangleApplication {
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}
private:
	GLFWwindow* window;

	//Instance e a conexao entre a aplicacao e a Vulkan library
	VkInstance instance;

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	//Placa de video a ser selecionada
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	void initWindow() {
		glfwInit();
		//Criar sem contexto do OpenGL
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		//Por enquanto desativar que a janela e redimensionavel
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Test", nullptr, nullptr);
	}

	void initVulkan() {
		createInstance();
		pickPhysicalDevice();
	}

	//Logica para dar maior pontuacao a melhor GPU encontrada no sistema e escolher ela como candidata
	bool rateDeviceSuitability(VkPhysicalDevice device) {
		int score = 0;

		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;

		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		//GPUs discretas possuem vantagem de performance
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			score += 1000;
		}

		//Tamanho maximo da textura afeta a qualidade dos graficos
		score += deviceProperties.limits.maxImageDimension2D;

		//Aplicacao nao pode funcionar sem geometry shaders
		if (!deviceFeatures.geometryShader) {
			return 0;
		}



	}

	void pickPhysicalDevice() {
		uint32_t deviceCount = 0;
		//Lista a quantidade de placas de video
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0) {
			 throw std::runtime_error("failed to find GPUs with Vulkan support");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		//Usa um mapa ordenado pra automaticamente organizar os candidatos por score crescente
		std::multimap<int, VkPhysicalDevice> candidates;

		for (const auto& device : devices) {
			int score = rateDeviceSuitability(device);
			candidates.insert(std::make_pair(score, device));
		}

		//Checa se o melhor candidato pode ser elegido
		if (candidates.rbegin()->first > 0) {
			physicalDevice = candidates.rbegin()->second;
		}
		else {
			throw std::runtime_error("failed to find a suitable GPU");
		}

	}

	//Segue o seguinte padrao:
	//1 - Ponteiro pra struct com info da criacao (createInfo.pApplicationInfo = &appInfo)
	//2 - Ponteiro pra callbacks alocadas customizadas, nullptr nesse tutorial (VkResult result = vkCreateInstance(, nullptr, ))
	//3 - Ponteiro pra variavel que armazena o identificador do novo objeto (VkResult result = vkCreateInstance(&createInfo, , &instance))
	void createInstance() {

		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested but not available");
		}
		
		//Struct com informacao sobre nossa aplicacao. Opcional, mas passa informacao util pro driver pra otimizar a aplicacao
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;
		
		//Essa struct nao e opcional e diz pro driver do Vulkan quais extensoes globais e camadas de validacao nos queremos usar
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		//Vulkan é uma API independente de plataforma, o que significa que você precisa de uma extensão para interagir com o sistema de janelas.
		//GLFW possui uma função integrada que retorna a(s) extensão(ões) necessária(s) para fazer isso, que podemos passar para a struct
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;

		//Quantidade de camadas globais de validacao para ativar
		createInfo.enabledLayerCount = 0;

		//Finalmente especificamos tudo que o Vulkan precisa pra criar uma instancia e agora vamos criar ela
		VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

		//Se estiverem ativos, inclui as camadas de validacao no create info
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}

	}

	//Checa se todas as camadas de validacao estao disponiveis
	bool checkValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}
			if (!layerFound) {
				return false;
			}
		}

		return true;
	}

	void mainLoop() {
		//Enquanto nao tem um erro a janela nao fecha
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}
	}

	void cleanup() {
		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);

		glfwTerminate();
	}
};

int main() {
	HelloTriangleApplication app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}