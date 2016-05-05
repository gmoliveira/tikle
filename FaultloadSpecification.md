# Faultload Specification #



A estrutura interna do faultload pode ser vista em [Faultload Structure](FaultloadStructure.md).

## Particionamento de Rede ##
```
@declare {10.0.0.1, 10.0.0.2, 10.0.0.3};

start:
  after (5s) do
    partition {10.0.0.1} {10.0.0.2, 10.0.0.3};
  end
  after (10s) do
    partition {10.0.0.1} {10.0.0.2} {10.0.0.3};
  end
  after(15s) do
    partition {10.0.0.1, 10.0.0.2} {10.0.0.3};
  end
stop:
  after (20s);
```

## Descarte/Duplicação de Pacotes ##
```
@10.0.0.1
start:
  after (5s) do
    tcp drop 5%;
  end
  after(10s) do
    udp drop 3%;
  end
stop:
  after (15s);

@10.0.0.2
start:
  after (5s) do
    tcp duplicate 3%;
  end
stop:
  after (10s);

@10.0.0.3
start:
  after (5s) do
    udp duplicate 5%;
  end
  after (10s) do
    tcp drop 5%;
  end
stop:
  after (15s);
```

## Injeção de falhas em aplicações multiprotocolo ##
```
@10.0.0.1
start:
  after (5s) do
    for each do
      tcp drop 5%;
      udp drop 3%;
    end
  end
  after(10s) do
    for each do
      tcp duplicate 3%;
      udp duplicate 4%;
    end
  end
stop:
  after (15s);

@10.0.0.2
start:
  after (5s) do
    for each do
      tcp drop 4%;
      udp drop 2%;
    end
  end
stop:
  after (10s);

@10.0.0.3
start:
  after (15s) do
    for each do
      tcp duplicate 3%;
      udp duplicate 4%;
    end
  end
stop:
  after (20s);
```

## Atraso de Pacotes ##
```
@10.0.0.1
start:
  after (5s) do
    tcp delay 5% for 2ms;;
  end
stop:
  after (10s);

@10.0.0.2
start:
  after (5s) do
    udp delay 3% for 3ms;
  end
stop:
  after (10s);

@10.0.0.3
start:
  after (5s) do
    tcp delay 5% for 4ms;
  end
stop:
  after (10s);
```

## Injeção de falhas sem gatilhos de início e fim ##
```
@10.0.0.1
start:
  tcp drop 5%;
stop:
  manual;

@10.0.0.2
start:
  udp duplicate 3%;
stop:
  manual;

@10.0.0.3
start:
  udp drop 3%;
stop:
  manual;
```


---


## Extensions ##

### progressive ###
Para caracterização de falhas em ambientes móveis

### if-then-else ###
Para verificação do estado local da máquina

```
if () then
  ...
else
  ...
end if
```

### SET ###
Para armazenar flags especificas de uma aplicacao