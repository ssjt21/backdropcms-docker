FROM php:7.4-apache
ADD ./apache2-foreground /usr/local/bin/apache2-foreground
ADD ./php.ini /usr/local/etc/php/php.ini
ADD ./allow_users /etc/allow_users
ADD ./backdrop /var/www/html
RUN apt-get update && \
      apt-get install -y curl \
      wget \
      libfreetype6-dev \
      zlib1g-dev \
      libzip-dev \
      vsftpd \
      vim &&\
      chmod 777  /usr/local/bin/apache2-foreground &&\
      service vsftpd start &&\
      useradd -d /home/ftp2 -s /bin/bash -m ftp2   &&\
      echo "ftp2:123456" |chpasswd   &&\
      chmod -R 777 /var/www/html &&\
      docker-php-ext-install pdo pdo_mysql bcmath mysqli gd zip
ADD ./vsftpd.conf /etc/vsftpd.conf

EXPOSE 80 21