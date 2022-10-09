
### 环境
```
ftp用户：
ftp2/123456



# docker-compose.yml

version: "2"
services:
  web:
    image: ssjt/backdropcms1.22.0
    depends_on:
      - mysql
    ports:
      - "8000:80"
    # volumes:
    #   - ./backdrop/:/var/www/html
    networks:
      - vuln-network
  mysql:
    image: mysql:5.6
    environment:
      - MYSQL_ROOT_PASSWORD=root
      - MYSQL_DATABASE=cms
    networks:
      - vuln-network
networks:
  vuln-network:
    external: true
```