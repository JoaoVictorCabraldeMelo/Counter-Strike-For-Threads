#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>

#define TERRORISTA 5
#define CONTRATERRORISTA 5
#define NRO_DE_JOGADORES TERRORISTA + CONTRATERRORISTA
#define ANDANDO 0
#define BOMBA_PLANTADA 2
#define BOMBA_DESARMADA 3
#define MORTO 6
#define ACHA_INIMIGO(upper, lower) ((rand() % (upper - lower + 1)) + lower)
#define CALCULA_DANO (rand() % 101)
#define MUDA_ESTADO(n, state) (estados[n].estado = state)

typedef struct jogador
{
  int estado;
  int vida;
} jogador;

jogador estados[NRO_DE_JOGADORES];

int contador_terroristas = TERRORISTA;
int contador_contraterrorista = CONTRATERRORISTA;
int contador_rodadas_terrorista = 0;
int contador_rodadas_contraterrorista = 0;
int estado_bomba = 0;

pthread_mutex_t lock_player = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_estado = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_contador = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_terrorista = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_contraterrorista = PTHREAD_MUTEX_INITIALIZER;


void *terrorista(void *arg);
void *contraterrorista(void *arg);
void procura_combate(int n);
void mostra_dano(int jogador, int adversario, int dano);
void reseta_rodada();
void vencedor_da_rodada(int nro_terroristas, int nro_contraterroristas);
void planta_bomba(int id_terrorista);
void desarma_bomba(int id_contraterrorista);

int main()
{
  int *id;

  srand(time(0));

  pthread_t jogadores[NRO_DE_JOGADORES];

  for (int i = 0; i < NRO_DE_JOGADORES; i++)
  {
    id = (int *)malloc(sizeof(int));
    *id = i;
    if (i < TERRORISTA)
    {
      pthread_create(&jogadores[i], NULL, terrorista, (void *)(id));
    }
    else if (i >= CONTRATERRORISTA)
    {
      pthread_create(&jogadores[i], NULL, contraterrorista, (void *)(id));
    }
  }

  pthread_join(jogadores[0], NULL);
  return 0;
}

void *terrorista(void *arg)
{
  int n = *((int *)arg);
  MUDA_ESTADO(n, ANDANDO);
  while (1)
  {
    sleep(rand() % 10);
    pthread_mutex_lock(&lock_player);
    if (contador_rodadas_terrorista == 15 || contador_rodadas_contraterrorista == 15)
    {
      break;
    }
    if (estados[n].estado != MORTO && contador_contraterrorista > 0)
    {

      printf("Terrorista %d\n", n);
      procura_combate(n);
    }
    if (rand() % 15 == 7)
    {
      pthread_mutex_lock(&lock_terrorista);
      planta_bomba(n);
      pthread_mutex_unlock(&lock_terrorista);
    }
    if (contador_contraterrorista == 0 || contador_terroristas == 0)
    {
      reseta_rodada();
    }
    pthread_mutex_unlock(&lock_player);
  }
  pthread_exit(0);
}

void *contraterrorista(void *arg)
{
  int n = *((int *)arg);
  MUDA_ESTADO(n, ANDANDO);
  while (1)
  {
    sleep(rand() % 10);
    pthread_mutex_lock(&lock_player);
    if (contador_rodadas_contraterrorista >= 15 || contador_rodadas_terrorista >= 15)
    {
      break;
    }
    if (estados[n].estado != MORTO && contador_terroristas > 0)
    {
      printf("Contra-Terrorista %d\n", n);
      procura_combate(n);
    }
    if (estado_bomba == BOMBA_PLANTADA && contador_terroristas == 0 && contador_contraterrorista > 0)
    {
      pthread_mutex_lock(&lock_contraterrorista);
      desarma_bomba(n);
      pthread_mutex_unlock(&lock_contraterrorista);
    }
    if (contador_contraterrorista == 0 || contador_contraterrorista == 0)
    {
      reseta_rodada();
    }
    pthread_mutex_unlock(&lock_player);
  }
  pthread_exit(0);
}

void procura_combate(int n)
{
  /*Calcula o dano que o tiro deu*/
  int dano = CALCULA_DANO;
  int jogador_atacado = 0;

  /*Verfica se o jogador é terrorista ou contraterrorista*/
  if (n < TERRORISTA)
  {
    /*Procura por contraterrorista*/
    jogador_atacado = ACHA_INIMIGO(NRO_DE_JOGADORES, TERRORISTA);

    /*Procura jogador atacado que não esteja morto e pode continuar procurando enquanto não estiver morto*/
    while (estados[jogador_atacado].estado == MORTO)
    {
      jogador_atacado = ACHA_INIMIGO(NRO_DE_JOGADORES, TERRORISTA);
    }
  }
  else
  {
    /*Procura por terrorista*/
    jogador_atacado = ACHA_INIMIGO(TERRORISTA, 0);

    /*Procura jogador atacado que não esteja morto e pode continuar procurando enquanto não estiver morto*/
    while (estados[jogador_atacado].estado == MORTO)
    {
      jogador_atacado = ACHA_INIMIGO(TERRORISTA, 0);
    }
  }

  /* Coloca o dano no jogador atacado*/
  if (jogador_atacado < TERRORISTA && contador_terroristas > 0)
  {
    estados[jogador_atacado].vida += dano;
    mostra_dano(n, jogador_atacado, dano);
  }
  else if (contador_contraterrorista > 0)
  {
    estados[jogador_atacado].vida += dano;
    mostra_dano(n, jogador_atacado, dano);
  }

  /*Atualiza os dados e contador da equipe*/
  if (estados[jogador_atacado].vida >= 100 && estados[jogador_atacado].estado != MORTO && estados[n].estado != MORTO)
  {
    pthread_mutex_lock(&lock_estado);
    MUDA_ESTADO(jogador_atacado, MORTO);
    pthread_mutex_unlock(&lock_estado);
    pthread_mutex_lock(&lock_contador);
    if (jogador_atacado < TERRORISTA && contador_terroristas > 0)
    {
      /*Aqui pode ocorrer data races*/
      contador_terroristas -= 1;
    }
    else if (contador_contraterrorista > 0)
    {
      /*Aqui pode ocorrer data races*/
      contador_contraterrorista -= 1;
    }
    pthread_mutex_unlock(&lock_contador);
  }
  printf("\nContra-Terrorista: %d \tTerrorista: %d \n", contador_contraterrorista, contador_terroristas);
}

void mostra_dano(int jogador, int adversario, int dano)
{
  /*Mostra dano e o jogador atacado*/
  if (jogador < TERRORISTA)
  {
    printf("\nTerrorista %d atirou no Contra-Terrorista %d e deu dano de %d \n", jogador, adversario, dano);
  }
  else
  {
    printf("\nContra-Terrorista %d atirou no Terrorista %d e deu dano de %d \n", jogador, adversario, dano);
  }
}

void reseta_rodada()
{
  for (int i = 0; i < NRO_DE_JOGADORES; i++)
  {
    MUDA_ESTADO(i, ANDANDO);
    estados[i].vida = 0;
  }

  pthread_mutex_lock(&lock_contador);
  vencedor_da_rodada(contador_terroristas, contador_contraterrorista);
  contador_contraterrorista = CONTRATERRORISTA;
  contador_terroristas = TERRORISTA;
  estado_bomba = 0;
  pthread_mutex_unlock(&lock_contador);
}

void vencedor_da_rodada(int nro_terroristas, int nro_contraterrorista)
{
  if (nro_terroristas > 0)
  {
    contador_rodadas_terrorista += 1;
    printf("\n\n\nTerroristas Venceram \n Rodadas Terroristas: %d \tRodadas Contra-Terrorista: %d\n\n\n", contador_rodadas_terrorista, contador_rodadas_contraterrorista);
  }
  else if (nro_contraterrorista > 0)
  {
    contador_rodadas_contraterrorista += 1;
    printf("\n\n\nContra-Terroristas Venceram\n Rodadas Terroristas: %d \tRodadas Contra-Terrorista: %d\n\n\n", contador_rodadas_terrorista, contador_rodadas_contraterrorista);
  }
}

void planta_bomba(int id_terrorista)
{
  estado_bomba = BOMBA_PLANTADA;
  printf("Terrorista %d esta plantando a bomba\n", id_terrorista);
  sleep(15);
}

void desarma_bomba(int id_contraterrorista)
{
  estado_bomba = BOMBA_DESARMADA;
  printf("ContraTerrorista %d esta desarmando a bomba\n", id_contraterrorista);
  sleep(10);
}