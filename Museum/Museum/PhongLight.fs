#version 330 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 FragPos;  

uniform vec3 lightPos; 
uniform vec3 viewPos; 
uniform vec3 lightColor;
uniform vec3 objectColor;

uniform float Ka;
uniform float Kd;
uniform float Ks;
uniform int n;

void main()
{
    vec3 ambient = lightColor * Ka;
    
    vec3 normal = normalize(Normal);
    vec3 lightDirection = normalize(lightPos - FragPos);
    vec3 diffuse = lightColor * Kd * max(dot(normal, lightDirection), 0.0);
    
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDirection, normal);
    float specFactor = pow(max(dot(viewDir, reflectDir), 0.0), n);
    vec3 specular = Ks * specFactor * lightColor; 
    

    vec3 result = objectColor * (ambient + diffuse + specular);

    FragColor = vec4(result, 1.0);
} 
