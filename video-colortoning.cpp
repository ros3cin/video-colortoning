#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <cmath>
#include <cstring>
#define RED 2
#define GREEN 1
#define BLUE 0
#define PIXEL(i,j,colunas) ((i*colunas*3)+(j*3))
#define truncar_uchar(a, b) \
    if (((int)(a)) + (b) < 0) \
        (a) = 0; \
    else if (((int)(a)) + (b) > 255) \
        (a) = 255; \
    else \
        (a) += (b);

using namespace cv;

//tabela onde havera o pre-calculo da cor mais proxima conforme a paleta escolhida
int luTable[256][256][256];
int pixelPos[3000][3000];
int calcErro[256][8];

//na ordem: preto(0) - branco(1) - vermelho(2) - verde(3) - azul(4) - ciano(5) - magenta(6) - amarelo(7)
Vec3b paleta3bit[8]={Vec3b(0,0,0),Vec3b(255,255,255),Vec3b(255,0,0),Vec3b(0,255,0),Vec3b(0,0,255),Vec3b(0,255,255),Vec3b(255,0,255),Vec3b(255,255,0)};

void inicializarErros()
{
    for(int i = 0; i < 256; i++)
    {
        for(int j = 0; j < 8; j++)
        {
            calcErro[i][j]=(i*j) >> 4;
        }
    }
}

int calcularErro(int erro, int numerador)
{
    if(erro < 0)
    {
        erro = -erro;
        return (-calcErro[erro][numerador]);
    }
    else
        return calcErro[erro][numerador];
}

int calcularIndiceMBVQ(int r, int g, int b, int quadrupla[4])
{
    int menorDistancia = (255*255*3)+1;
    int indiceCorEscolhida = 0;
    for (int i = 0; i < 4; i++)
    {
        int dR = r - paleta3bit[quadrupla[i]][0];
        int dG = g - paleta3bit[quadrupla[i]][1];
        int dB = b - paleta3bit[quadrupla[i]][2];
        int dist = (dR*dR) + (dG*dG) + (dB*dB);
        if (dist < menorDistancia)
        {
            menorDistancia = dist;
            indiceCorEscolhida = quadrupla[i];
        }
    }
    return indiceCorEscolhida;
}

//Minimum Brightness Variation Quadruple
int calcularMBVQ(int r, int g, int b)
{
    int black=0, white=1, red=2, green=3, blue=4, ciano=5, magenta=6, yellow=7;
    int cmyw[4]={ciano,magenta,yellow,white};
    int mygc[4]={magenta,yellow,green,ciano};
    int rgmy[4]={red,green,magenta,yellow};

    int krgb[4]={black,red,green,blue};
    int rgbm[4]={red,green,blue,magenta};
    int cmgb[4]={ciano,magenta,green,blue};

    if((r+g)>255)
        if((g+b)>255)
            if((r+g+b)>510)
                return calcularIndiceMBVQ(r,g,b,cmyw);
            else
                return calcularIndiceMBVQ(r,g,b,mygc);
        else
            return calcularIndiceMBVQ(r,g,b,rgmy);
    else
        if ( !((g+b)>255) )
            if ( !((r+g+b)>255) )
                return calcularIndiceMBVQ(r,g,b,krgb);
            else
                return calcularIndiceMBVQ(r,g,b,rgbm);
        else
            return calcularIndiceMBVQ(r,g,b,cmgb);

}

int calcularCorMaisProxima(int r, int g, int b)
{
    int menorDistancia = (255*255*3)+1;
    int indiceCorEscolhida = 0;
    for (int i = 0; i < 8; i++)
    {
        int dR = r - paleta3bit[i][0];
        int dG = g - paleta3bit[i][1];
        int dB = b - paleta3bit[i][2];
        int dist = (dR*dR) + (dG*dG) + (dB*dB);
        if (dist < menorDistancia)
        {
            menorDistancia = dist;
            indiceCorEscolhida = i;
        }
    }
    return indiceCorEscolhida;
}

void inicializarPosicoes(int linhas, int colunas)
{
    for(int i = 0; i < linhas;i++)
        for (int j = 0; j < colunas; j++)
            pixelPos[i][j]= (i*colunas*3) + (j*3);
}

void inicializarMatriz()
{
    for (int i = 0; i < 256; i++)
        for (int j = 0; j < 256; j++)
            for (int k = 0; k < 256; k++)
                //luTable[i][j][k] = calcularCorMaisProxima(i,j,k);//sem criterio de menor variacao luminosa
                luTable[i][j][k] = calcularMBVQ(i,j,k); //com criterio de menor variacao luminosa
}


Mat& aplicarFloydSteinbergEff2(Mat& imagem)
{
    int linhas = imagem.rows;
    int colunas = imagem.cols * 3;
    bool direita=true;
    if(imagem.isContinuous())
    {
        int inicio = 0;
        int fim = colunas;
        int prxPixel = 3;
        int antPixel = -3;
        int prxLinha = colunas;
        int red = 2, green = 1, blue = 0;
        int totalColunas = linhas * colunas;
        int i;
        uchar* ptCanal;
        ptCanal = imagem.ptr<uchar>(0);
        while(inicio < totalColunas)
        {
            if(direita)
            {
                for(i=inicio;i<fim;i+=3)
                {
                    int corEscolhida = luTable[ptCanal[i+red]][ptCanal[i+green]][ptCanal[i+blue]];
                    int erro[3];
                    erro[0] = ptCanal[i+red] - paleta3bit[corEscolhida][0];
                    erro[1] = ptCanal[i+green] - paleta3bit[corEscolhida][1];
                    erro[2] = ptCanal[i+blue] - paleta3bit[corEscolhida][2];

                    ptCanal[i+red] = paleta3bit[corEscolhida][0];
                    ptCanal[i+green] = paleta3bit[corEscolhida][1];
                    ptCanal[i+blue] = paleta3bit[corEscolhida][2];

                    if( (i+prxPixel) < fim)
                    {
                        truncar_uchar(ptCanal[i+prxPixel+red] , ((erro[0]*7) >> 4 ));
                        truncar_uchar(ptCanal[i+prxPixel+green] , ((erro[1]*7) >> 4 ));
                        truncar_uchar(ptCanal[i+prxPixel+blue] , ((erro[2]*7) >> 4 ));
                    }

                    // para sul
                    if( (i+prxLinha) < totalColunas)
                    {
                        truncar_uchar(ptCanal[i+prxLinha+red] , ((erro[0]*5) >> 4 ));
                        truncar_uchar(ptCanal[i+prxLinha+green] , ((erro[1]*5) >> 4 ));
                        truncar_uchar(ptCanal[i+prxLinha+blue] , ((erro[2]*5) >> 4 ));
                    }

                    // sudoeste
                    if( ((i+antPixel)>=inicio) && ((i+prxLinha)<totalColunas) )
                    {
                        truncar_uchar(ptCanal[i+antPixel+prxLinha+red] , ((erro[0]*3) >> 4 ));
                        truncar_uchar(ptCanal[i+antPixel+prxLinha+green] , ((erro[1]*3) >> 4 ));
                        truncar_uchar(ptCanal[i+antPixel+prxLinha+blue] , ((erro[2]*3) >> 4 ));
                    }

                    // sudeste
                    if( ((i+prxPixel)<fim) && ((i+prxLinha)<totalColunas) )
                    {
                        truncar_uchar(ptCanal[i+prxPixel+prxLinha+red] , ((erro[0]*1) >> 4 ));
                        truncar_uchar(ptCanal[i+prxPixel+prxLinha+green] , ((erro[1]*1) >> 4 ));
                        truncar_uchar(ptCanal[i+prxPixel+prxLinha+blue] , ((erro[2]*1) >> 4 ));
                    }

                }
            }
            else
            {
                for(i=fim-3;i>=inicio;i-=3)
                {
                    int corEscolhida = luTable[ptCanal[i+red]][ptCanal[i+green]][ptCanal[i+blue]];
                    int erro[3];
                    erro[0] = ptCanal[i+red] - paleta3bit[corEscolhida][0];
                    erro[1] = ptCanal[i+green] - paleta3bit[corEscolhida][1];
                    erro[2] = ptCanal[i+blue] - paleta3bit[corEscolhida][2];

                    ptCanal[i+red] = paleta3bit[corEscolhida][0];
                    ptCanal[i+green] = paleta3bit[corEscolhida][1];
                    ptCanal[i+blue] = paleta3bit[corEscolhida][2];

                    if((i+antPixel) >= inicio)
                    {
                        truncar_uchar(ptCanal[i+antPixel+red] , ((erro[0]*7) >> 4 ));
                        truncar_uchar(ptCanal[i+antPixel+green] , ((erro[1]*7) >> 4 ));
                        truncar_uchar(ptCanal[i+antPixel+blue] , ((erro[2]*7) >> 4 ));
                    }

                    // para sul
                    if((i+prxLinha) < totalColunas)
                    {
                        truncar_uchar(ptCanal[i+prxLinha+red] , ((erro[0]*5) >> 4 ));
                        truncar_uchar(ptCanal[i+prxLinha+green] , ((erro[1]*5) >> 4 ));
                        truncar_uchar(ptCanal[i+prxLinha+blue] , ((erro[2]*5) >> 4 ));
                    }

                    // sudoeste
                    if( ((i+antPixel)>=inicio) && ((i+prxLinha)<totalColunas) )
                    {
                        truncar_uchar(ptCanal[i+antPixel+prxLinha+red] , ((erro[0]*1) >> 4 ));
                        truncar_uchar(ptCanal[i+antPixel+prxLinha+green] , ((erro[1]*1) >> 4 ));
                        truncar_uchar(ptCanal[i+antPixel+prxLinha+blue] , ((erro[2]*1) >> 4 ));
                    }

                    // sudeste
                    if( ((i+prxPixel)<fim) && ((i+prxLinha)<totalColunas) )
                    {
                        truncar_uchar(ptCanal[i+prxPixel+prxLinha+red] , ((erro[0]*3) >> 4 ));
                        truncar_uchar(ptCanal[i+prxPixel+prxLinha+green] , ((erro[1]*3) >> 4 ));
                        truncar_uchar(ptCanal[i+prxPixel+prxLinha+blue] , ((erro[2]*3) >> 4 ));
                    }
                }
            }
            direita=!direita;
            inicio += colunas;
            fim += colunas;
        }
    } else {
        printf("Alerta! Nao coube na memória!\n");
    }
    return imagem;
}

int diferencaDeCor(const uchar *rgbAtual,const uchar *rgbAnterior)
{
    int diferenca=0;
    diferenca = abs((rgbAtual[0]-rgbAnterior[0])+(rgbAtual[1]-rgbAnterior[1])+(rgbAtual[2]-rgbAnterior[2]));
    //printf("%d %u %u\n",diferenca,rgbAtual[1],rgbAnterior[1]);
    return diferenca;
}
Mat& aplicarFloydSteinbergEff2(Mat& imagem, Mat& imgAnterior, int limiar)
{
    int linhas = imagem.rows;
    int colunas = imagem.cols * 3;
    bool direita=true;
    if(imagem.isContinuous())
    {
        int inicio = 0;
        int fim = colunas;
        int prxPixel = 3;
        int antPixel = -3;
        int prxLinha = colunas;
        int red = 2, green = 1, blue = 0;
        int totalColunas = linhas * colunas;
        int i;
        uchar* ptCanal;
        uchar* ptCanalAnt;
        ptCanal = imagem.ptr<uchar>(0);
        ptCanalAnt = imgAnterior.ptr<uchar>(0);
        while(inicio < totalColunas)
        {
            if(direita)
            {
                for(i=inicio;i<fim;i+=3)
                {
                    if(diferencaDeCor( ptCanal+i, ptCanalAnt+i ) < limiar)
                    {
                        ptCanal[i+red] = ptCanalAnt[i+red];
                        ptCanal[i+green] = ptCanalAnt[i+green];
                        ptCanal[i+blue] = ptCanalAnt[i+blue];
                        continue;
                    }
                    int corEscolhida = luTable[ptCanal[i+red]][ptCanal[i+green]][ptCanal[i+blue]];
                    int erro[3];
                    erro[0] = ptCanal[i+red] - paleta3bit[corEscolhida][0];
                    erro[1] = ptCanal[i+green] - paleta3bit[corEscolhida][1];
                    erro[2] = ptCanal[i+blue] - paleta3bit[corEscolhida][2];

                    ptCanal[i+red] = paleta3bit[corEscolhida][0];
                    ptCanal[i+green] = paleta3bit[corEscolhida][1];
                    ptCanal[i+blue] = paleta3bit[corEscolhida][2];

                    if( (i+prxPixel) < fim)
                    {
                        truncar_uchar(ptCanal[i+prxPixel+red] , ((erro[0]*7) >> 4 ));
                        truncar_uchar(ptCanal[i+prxPixel+green] , ((erro[1]*7) >> 4 ));
                        truncar_uchar(ptCanal[i+prxPixel+blue] , ((erro[2]*7) >> 4 ));
                    }

                    // para sul
                    if( (i+prxLinha) < totalColunas)
                    {
                        truncar_uchar(ptCanal[i+prxLinha+red] , ((erro[0]*5) >> 4 ));
                        truncar_uchar(ptCanal[i+prxLinha+green] , ((erro[1]*5) >> 4 ));
                        truncar_uchar(ptCanal[i+prxLinha+blue] , ((erro[2]*5) >> 4 ));
                    }

                    // sudoeste
                    if( ((i+antPixel)>=inicio) && ((i+prxLinha)<totalColunas) )
                    {
                        truncar_uchar(ptCanal[i+antPixel+prxLinha+red] , ((erro[0]*3) >> 4 ));
                        truncar_uchar(ptCanal[i+antPixel+prxLinha+green] , ((erro[1]*3) >> 4 ));
                        truncar_uchar(ptCanal[i+antPixel+prxLinha+blue] , ((erro[2]*3) >> 4 ));
                    }

                    // sudeste
                    if( ((i+prxPixel)<fim) && ((i+prxLinha)<totalColunas) )
                    {
                        truncar_uchar(ptCanal[i+prxPixel+prxLinha+red] , ((erro[0]*1) >> 4 ));
                        truncar_uchar(ptCanal[i+prxPixel+prxLinha+green] , ((erro[1]*1) >> 4 ));
                        truncar_uchar(ptCanal[i+prxPixel+prxLinha+blue] , ((erro[2]*1) >> 4 ));
                    }

                }
            }
            else
            {
                for(i=fim-3;i>=inicio;i-=3)
                {
                    if(diferencaDeCor( ptCanal+i, ptCanalAnt+i ) < limiar)
                    {
                        ptCanal[i+red] = ptCanalAnt[i+red];
                        ptCanal[i+green] = ptCanalAnt[i+green];
                        ptCanal[i+blue] = ptCanalAnt[i+blue];
                        continue;
                    }
                    int corEscolhida = luTable[ptCanal[i+red]][ptCanal[i+green]][ptCanal[i+blue]];
                    int erro[3];
                    erro[0] = ptCanal[i+red] - paleta3bit[corEscolhida][0];
                    erro[1] = ptCanal[i+green] - paleta3bit[corEscolhida][1];
                    erro[2] = ptCanal[i+blue] - paleta3bit[corEscolhida][2];

                    ptCanal[i+red] = paleta3bit[corEscolhida][0];
                    ptCanal[i+green] = paleta3bit[corEscolhida][1];
                    ptCanal[i+blue] = paleta3bit[corEscolhida][2];

                    if((i+antPixel) >= inicio)
                    {
                        truncar_uchar(ptCanal[i+antPixel+red] , ((erro[0]*7) >> 4 ));
                        truncar_uchar(ptCanal[i+antPixel+green] , ((erro[1]*7) >> 4 ));
                        truncar_uchar(ptCanal[i+antPixel+blue] , ((erro[2]*7) >> 4 ));
                    }

                    // para sul
                    if((i+prxLinha) < totalColunas)
                    {
                        truncar_uchar(ptCanal[i+prxLinha+red] , ((erro[0]*5) >> 4 ));
                        truncar_uchar(ptCanal[i+prxLinha+green] , ((erro[1]*5) >> 4 ));
                        truncar_uchar(ptCanal[i+prxLinha+blue] , ((erro[2]*5) >> 4 ));
                    }

                    // sudoeste
                    if( ((i+antPixel)>=inicio) && ((i+prxLinha)<totalColunas) )
                    {
                        truncar_uchar(ptCanal[i+antPixel+prxLinha+red] , ((erro[0]*1) >> 4 ));
                        truncar_uchar(ptCanal[i+antPixel+prxLinha+green] , ((erro[1]*1) >> 4 ));
                        truncar_uchar(ptCanal[i+antPixel+prxLinha+blue] , ((erro[2]*1) >> 4 ));
                    }

                    // sudeste
                    if( ((i+prxPixel)<fim) && ((i+prxLinha)<totalColunas) )
                    {
                        truncar_uchar(ptCanal[i+prxPixel+prxLinha+red] , ((erro[0]*3) >> 4 ));
                        truncar_uchar(ptCanal[i+prxPixel+prxLinha+green] , ((erro[1]*3) >> 4 ));
                        truncar_uchar(ptCanal[i+prxPixel+prxLinha+blue] , ((erro[2]*3) >> 4 ));
                    }
                }
            }
            direita=!direita;
            inicio += colunas;
            fim += colunas;
        }
    } else {
        printf("Alerta! Nao coube na memória!\n");
    }
    return imagem;
}

Mat& computarDiferencas(Mat& imagem, Mat& imgAnterior, int limiar)
{
    int linhas = imagem.rows;
    int colunas = imagem.cols * 3;
    bool direita=true;
    if(imagem.isContinuous())
    {
        int inicio = 0;
        int fim = colunas;
        int prxPixel = 3;
        int antPixel = -3;
        int prxLinha = colunas;
        int red = 2, green = 1, blue = 0;
        int totalColunas = linhas * colunas;
        int i;
        uchar* ptCanal;
        uchar* ptCanalAnt;
        ptCanal = imagem.ptr<uchar>(0);
        ptCanalAnt = imgAnterior.ptr<uchar>(0);
        while(inicio < totalColunas)
        {
            
            for(i=inicio;i<fim;i+=3)
            {
                if(diferencaDeCor( ptCanal+i, ptCanalAnt+i ) < limiar)
                {
                    ptCanal[i+red] = ptCanalAnt[i+red];
                    ptCanal[i+green] = ptCanalAnt[i+green];
                    ptCanal[i+blue] = ptCanalAnt[i+blue];
                }
            }
            
            inicio += colunas;
            fim += colunas;
        }
    } else {
        printf("Alerta! Nao coube na memória!\n");
    }
    return imagem;
}


int main(int argc, char** argv )
{
    if ( argc != 2 )
    {
        printf("Informe a imagem a ser processada\n");
        return -1;
    }

    Mat image;
    
    image = imread( argv[1], 1 );

    if ( !image.data )
    {
        printf("Nao e uma imagem \n");
        return -1;
    }

    Mat resultado;
    inicializarMatriz();
    inicializarPosicoes(image.rows,image.cols);
    inicializarErros();

    int framesCount = 100;

    double t = (double)getTickCount();
    
    for(int i = 0; i < framesCount; i++)
    {
        Mat clone1 = image.clone();
        resultado = aplicarFloydSteinbergEff2(clone1);
        clone1.release();
    }
    
    t = ((double)getTickCount() - t)/getTickFrequency();
    printf("Tempo com efficient: %f fps\n",framesCount/t);
    
    //Para depois
    //Mat cloneResultado = resultado.clone();
    //aussianBlur(resultado,cloneResultado,Size(3,3),3,3);

    
    
    /*namedWindow("FloydSteinberg", CV_WINDOW_AUTOSIZE );
    imshow("FloydSteinberg", resultado);

    namedWindow("Gaussiano", CV_WINDOW_AUTOSIZE );
    imshow("Gaussiano", cloneResultado);*/

    
    vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
    compression_params.push_back(9);
    imwrite("resultado.bmp",resultado,compression_params);

    vector<int> compression_params2;
    compression_params2.push_back(CV_IMWRITE_JPEG_QUALITY);
    compression_params2.push_back(95);
    imwrite("resultadoPeg-95.jpeg",image,compression_params2);

    vector<int> compression_params3;
    compression_params3.push_back(CV_IMWRITE_JPEG_QUALITY);
    compression_params3.push_back(70);
    imwrite("resultadoPeg-70.jpeg",image,compression_params3);

    vector<int> compression_params4;
    compression_params4.push_back(CV_IMWRITE_JPEG_QUALITY);
    compression_params4.push_back(70);
    imwrite("resultadoPeg-colortone-70.jpeg",resultado,compression_params4);

    
    /*VideoCapture cap("./video_beth.mp4"); // open the default camera

    inicializarPosicoes((int)cap.get(CV_CAP_PROP_FRAME_WIDTH),(int)cap.get(CV_CAP_PROP_FRAME_HEIGHT));//pre-calcula cor

    if(!cap.isOpened())  
        return -1;
    VideoWriter escritor;
    escritor.open("./teste3.avi",CV_FOURCC('P','I','M','1'),30,Size((int)cap.get(CV_CAP_PROP_FRAME_WIDTH),(int)cap.get(CV_CAP_PROP_FRAME_HEIGHT)),true);
    
    Mat frameAnt,frameAtual;
    //namedWindow("Resultado",1);
    cap >> frameAnt;
    frameAnt = aplicarFloydSteinbergEff2(frameAnt);
    //imshow("Resultado", frameAnt);
    escritor.write(frameAnt);
    int fr = 0;
    char buf[50];
    for(;;)
    {
        cap >> frameAtual;
        
        //frameAtual = aplicarFloydSteinbergEff2(frameAtual);
        frameAtual = aplicarFloydSteinbergEff2(frameAtual,frameAnt,80);
        //frameAtual = computarDiferencas(frameAtual,frameAnt,20);
        GaussianBlur(frameAtual,frameAtual,Size(3,3),3,3);

        //imshow("Resultado", frameAtual);
        escritor.write(frameAtual);

        //sequencia de imagens
        
        //POSSO QUERER COMENTAR O BLOCO ABAIXO INDENTADO
            //vector<int> compression_params;
            //compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
            //compression_params.push_back(9);
            //sprintf(buf,"./sequencia/seq%d.bmp",fr++);
            //imwrite(buf,frameAtual,compression_params);


        //printf("Passou4!\n");
        frameAnt = frameAtual.clone();
        //if(waitKey(30) >= 0) break;
    }*/
    

    waitKey(0);

    return 0;
}
