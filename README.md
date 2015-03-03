# Para compilar e rodar o *video colortoning*

Este código foi compilado e rodado na distro Ubuntu, porém também deve funcionar no sistema operacional Windows.

Abaixo seguem as instruções no Linux.
- 1. Instalar o OpenCV segundo as instruções no link: http://docs.opencv.org/doc/tutorials/introduction/linux_install/linux_install.html
- 2. Cada um dos pacotes listado na página devem ser instalados como informados utilizando, no caso do Ubuntu, "sudo apt-get install [nome_do_pacote]"
- 3. Ao terminar a instalação, descarregar os arquivos deste repositório em alguma pasta
- 4. Abrir o termina na pasta e primeiramente realizar o comando "cmake -D CMAKE_BUILD_TYPE=RELEASE .". É importante utilizar a *build* de release para questões de performance. Por padrão está ativada a *build* de DEBUG, que possui performance inferior.
- 5. Após o cmake, digitar o comando "make". Aqui ocorrerá a compilação após acada modificação de código.
- 6. Após compilado, para rodar o código utilizar o comando "./video-colortoning [nome_de_alguma_imagem]"

