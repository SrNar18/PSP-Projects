"""Narcade: original campaign, compiled into a read-only C table."""
import json, pathlib

LOCATIONS = [
 ('Refugio de Nico',0,3),('Taller de Luna',1,4),('Cafe de Vera',3,3),
 ('Radio Ladera',0,1),('Estacion del Metro',4,2),('Archivo del Centro',4,3),
 ('Cancha de la 13',0,2),('Estadio',2,1),('Terminal del Norte',4,0),
 ('Bodega del Rio',5,4),('Mirador',1,0),('Torre Prisma',6,6),
 ('Mercado de Belen',1,6),('Parque de Laureles',2,3),('Hospital',3,5),
 ('Subestacion',6,2),('Cabina del Cable',0,0),('Imprenta',3,4),
 ('Galeria del Poblado',6,5),('Antena Oriental',7,1),('Deposito de buses',5,0),
 ('Puente de San Juan',4,4),('Plaza de la Luz',3,2),('Servidor Central',7,6),
 ('Taller de Vinilos',2,5),('Jardin del Sur',4,6),('Estudio Ritmo',1,2),
 ('Azotea de Sara',6,0),('Parque del Rio',4,5),('Observatorio',7,3)
]

# kind, location, objective, seed, parameter. Puzzle seeds vary every mission.
def S(kind,loc,text,seed=0,par=0): return dict(kind=kind,loc=loc,text=text,seed=seed,par=par)
M=[]
def mission(title,who,intro,outro,*steps):
 M.append(dict(title=title,who=who,intro=intro,outro=outro,steps=list(steps),reward=250+len(M)*35))

mission('Una entrega equivocada','Nico',
 'Volvi a Medellin para empezar de cero. Luna me presta su carro para una entrega. Pero el sobre lleva mi nombre... y una fecha de manana.',
 'Vera encuentra una orden de desalojo firmada con mi identidad. Alguien vende el barrio en nombre de gente que nunca firmo.',
 S('TALK',1,'Luna: hay carros en las calles. TRIANGULO para entrar. El verde del taller es prestado.'),
 S('DRIVE',2,'Consigue un carro y lleva el sobre al Cafe de Vera.'),
 S('CODE',2,'Abre el sobre digital: el archivo usa cuatro digitos.',173),
 S('TALK',0,'Vuelve al refugio. Vera guardo una copia de la orden.'))
mission('El nombre en la lista','Vera',
 'La firma falsa aparece en el Archivo del Centro. Quiero ver el original. Sara, mi hermana, trabajaba alli antes de desaparecer.',
 'No es solo tu firma. Hay cientos. En todos los expedientes aparece una empresa ficticia: Prisma.',
 S('TALK',5,'Habla con el archivista. Solo quedan terminales sin conexion.'),
 S('CIRCUIT',5,'Reconecta el PC del archivo para consultar las firmas.',22),
 S('MEMORY',5,'Reconstruye la secuencia del expediente de Sara.',12),
 S('TALK',17,'La imprenta puede conservar una copia fisica.'))
mission('Frecuencia de barrio','Mara',
 'Radio Ladera recibe mensajes de Sara mezclados con interferencia. Si arreglo el enlace, tal vez escuchemos su voz.',
 'Sara: No confien en las pantallas. Miren la ciudad. La primera llave esta donde el concreto aprendio a cantar.',
 S('TUNE',3,'Sintoniza la voz escondida en Radio Ladera.',34),
 S('RHYTHM',26,'Sincroniza el sample en el estudio de hip hop.',3),
 S('TALK',6,'Busca el mural de la cancha en la Comuna 13.'),
 S('CODE',6,'Resuelve la clave del mural con sus cuatro pistas.',421))
mission('Pintura fresca','Tiza',
 'Taparon el mural durante la noche. Tiza salvo los bocetos, pero su mochila quedo al otro lado del patio vigilado.',
 'El boceto contiene un mapa de apagones. No ocurren al azar: preparan calles completas para vaciarlas.',
 S('STEALTH',6,'Cruza el patio sin tocar los haces de vigilancia.',15),
 S('LOCK',6,'Abre el estuche de los bocetos.',8),
 S('DRIVE',13,'Lleva los bocetos al parque de Laureles.'),
 S('TALK',2,'Vera compara los apagones con los desalojos.'))
mission('La vuelta de Luna','Luna',
 'Un comprador tiene la lista de los camiones de Prisma. Solo negociara con quien gane su recorrido. No me gusta, pero necesitamos esa lista.',
 'Luna reconoce una matricula: es el camion que recogio los equipos del archivo la noche en que Sara desaparecio.',
 S('TALK',1,'Luna prepara el recorrido. Consigue un carro resistente.'),
 S('RACE',7,'Completa seis controles urbanos antes del limite.',5,115),
 S('LOCK',20,'Abre el casillero del conductor.',9),
 S('TALK',1,'Vuelve al taller con la lista de matriculas.'))
mission('La ciudad se apaga','Vera',
 'El siguiente corte empieza esta noche. El hospital esta en la zona. Podemos mantener la luz si llegamos a la subestacion a tiempo.',
 'El hospital sigue encendido. Prisma ya sabe que estamos aqui. Un mensaje aparece en mi telefono: Devuelve lo que no es tuyo.',
 S('DRIVE',14,'Lleva una bateria de emergencia al hospital.'),
 S('CIRCUIT',15,'Restablece el circuito de la subestacion.',14),
 S('CHASE',21,'Despista la persecucion y reduce la busqueda a cero.',2,18),
 S('TALK',0,'Reunete con Vera. Fin del capitulo I.'))

mission('Debajo del ruido','Mara',
 'La amenaza llevaba un ruido de fondo: frenos de bus y una campana. Mara cree que podemos encontrar donde grabaron el mensaje.',
 'El sonido viene del deposito del Norte. Un vigilante recuerda a Sara: ella entro caminando, sin que nadie la obligara.',
 S('TUNE',3,'Aisla la frecuencia de fondo del mensaje.',87),
 S('MEMORY',20,'Repite la secuencia de accesos del deposito.',41),
 S('TALK',8,'Pregunta por Sara en la terminal.'),
 S('TALK',27,'Busca una senal en la antigua azotea de Sara.'))
mission('La habitacion cerrada','Nico',
 'Sara dejo una caja en su azotea. La cerro con juegos que inventabamos de pequenos. Si la abro, sabre por que no volvio a casa.',
 'Una nota: Me infiltre en Prisma. Si desaparezco, busquen los tres nodos. No vengan a rescatarme sin pruebas.',
 S('LOCK',27,'Abre la caja de Sara.',24),
 S('CODE',27,'Descifra la clave del diario de Sara.',689),
 S('MEMORY',29,'Reconstruye las coordenadas de los tres nodos.',22),
 S('TALK',2,'Comparte el diario con Vera.'))
mission('Un favor al mercado','Dona Luz',
 'Dona Luz conoce al tecnico del nodo occidental. Su puesto pierde alimentos en cada apagon. Hoy toca devolver un favor al barrio.',
 'Luz entrega el plano del primer nodo. El tecnico escondio sus notas en un viejo vinilo para que Prisma no las borrara.',
 S('DRIVE',12,'Recoge las cajas refrigeradas del mercado.'),
 S('RACE',12,'Haz seis entregas por la ciudad sin agotar el tiempo.',12,130),
 S('TALK',24,'Pregunta por el vinilo azul.'),
 S('RHYTHM',24,'Lee el patron grabado en el surco del vinilo.',7))
mission('Nodo occidental','Tiza',
 'El primer nodo esta detras de la cabina del Cable. Tiza conoce el camino. Sus murales nos serviran de referencia.',
 'Conseguimos una tercera parte del registro maestro. Cada desalojo activa una transferencia a la misma cuenta opaca.',
 S('TALK',16,'Localiza el acceso de mantenimiento del Cable.'),
 S('STEALTH',16,'Evita los haces del corredor tecnico.',32),
 S('CIRCUIT',16,'Conecta el nodo occidental al terminal aislado.',65),
 S('CODE',16,'Extrae la primera llave del registro.',728),
 S('TALK',3,'Mara guarda una copia fuera de la red.'))
mission('La carrera del mensajero','Luna',
 'Un mensajero transporta los recibos originales. Le prometieron proteger a su familia, pero ahora quiere salir. Le hemos preparado una ruta.',
 'El mensajero deja los recibos y se va. La firma que autoriza las compras es de Cardenal, el director de Prisma.',
 S('DRIVE',8,'Recoge al mensajero en un carro.'),
 S('RACE',8,'Recorre los seis puntos seguros.',9,105),
 S('CHASE',28,'Pierde a quienes siguieron al mensajero.',4,22),
 S('TALK',14,'Deja al mensajero en el hospital.'))
mission('El precio de una firma','Vera',
 'Los recibos bastan para denunciar las compras, pero alguien filtra la denuncia. Esta noche vienen por la imprenta y sus copias.',
 'Salvamos los documentos. Mi nombre ya no es el unico en la lista de Prisma: ahora estan los de todos mis amigos.',
 S('LOCK',17,'Abre el deposito de la imprenta.',31),
 S('MEMORY',17,'Clasifica las copias antes de salir.',66),
 S('DRIVE',10,'Lleva los documentos al mirador.'),
 S('TALK',10,'Vera decide seguir. Fin del capitulo II.'))

mission('Luces sobre el rio','Sara',
 'Por fin llama Sara. Esta viva. No puede salir aun: encontro un plan para borrar las deudas de Prisma y culpar a los vecinos.',
 'Sara nos da una hora y una frecuencia. El nodo del rio contiene las rutas del dinero, pero abre solo durante el cambio de turno.',
 S('TUNE',10,'Encuentra la frecuencia privada de Sara.',45),
 S('DRIVE',9,'Acercate a la bodega del rio en un carro.'),
 S('STEALTH',9,'Cruza el patio durante el cambio de guardia.',61),
 S('TALK',9,'Recoge la tarjeta que Sara dejo en la reja.'))
mission('El nodo del agua','Nico',
 'La bodega parece abandonada. Por debajo pasan cables nuevos. El registro del rio esta protegido por un circuito independiente.',
 'El dinero no sale de la ciudad. Regresa como creditos impagables a los mismos barrios que quieren vaciar.',
 S('CIRCUIT',9,'Alimenta el segundo nodo.',97),
 S('CODE',9,'Abre el registro financiero.',213),
 S('MEMORY',9,'Copia las transferencias en el orden correcto.',58),
 S('TALK',2,'Vera une las primeras dos llaves.'))
mission('Bajo el puente','Tiza',
 'Un vecino vio a Prisma mover un servidor por San Juan. Tiza quiere fotografiar la etiqueta antes de que lo escondan.',
 'La etiqueta muestra el destino: Torre Prisma. El ultimo nodo esta dentro de su propia sede.',
 S('TALK',21,'Encuentra al vecino bajo el puente.'),
 S('STEALTH',21,'Llega al contenedor sin activar la vigilancia.',42),
 S('LOCK',21,'Abre la cubierta de la etiqueta.',47),
 S('DRIVE',17,'Lleva la evidencia a la imprenta.'))
mission('Noche de vinilos','Mara',
 'Necesitamos una invitacion a la galeria que financia Prisma. Mara toca esta noche alli. Puedo ayudarla en la consola y entrar como parte del equipo.',
 'Entre dos canciones oimos a Cardenal: el borrado se llama Horizonte. Se ejecutara cuando inauguren la torre.',
 S('RHYTHM',26,'Ensaya el primer tema del concierto.',11),
 S('DRIVE',18,'Lleva a Mara y los equipos a la galeria.'),
 S('RHYTHM',18,'Completa la sesion de la galeria.',19),
 S('TUNE',18,'Aisla la conversacion de Cardenal.',75))
mission('Un carro sin dueno','Luna',
 'Sara necesita que retiremos un carro de Prisma antes de que revisen su maletero. Dentro escondio una copia del programa Horizonte.',
 'Horizonte no solo borra archivos. Puede apagar semaforos, comunicaciones y bombas de agua para impedir que la gente salga.',
 S('STEALTH',11,'Alcanza el parqueadero de la torre.',89),
 S('DRIVE',1,'Consigue un carro y lleva el respaldo al taller.'),
 S('CHASE',1,'El rastreador alerta a los perseguidores. Despistalos.',3,24),
 S('CIRCUIT',1,'Desconecta el rastreador del respaldo.',111))
mission('La grieta','Vera',
 'La filtracion salio de nuestro refugio. Encuentro el comunicador de Luna conectado a Prisma. Ella insiste en que alguien lo copio.',
 'La copia se hizo antes de que Luna nos conociera. El traidor no esta en el taller: esta dentro del sistema que usamos para hablar.',
 S('MEMORY',0,'Compara las secuencias del comunicador.',91),
 S('CODE',1,'Abre el historial del dispositivo de Luna.',541),
 S('TUNE',3,'Mueve la radio a una banda limpia.',104),
 S('TALK',1,'Escucha a Luna. Fin del capitulo III.'))

mission('Plan en papel','Vera',
 'Abandonamos los telefonos. Para entrar en la torre usaremos planos, senales de radio y tres llaves fisicas. Cada persona carga solo una parte.',
 'El plano tiene una sala borrada. Sara cree que ahi mantienen el nodo oriental y el control del programa Horizonte.',
 S('TALK',17,'Recoge los planos impresos.'),
 S('CODE',17,'Interpreta las marcas del plano.',873),
 S('DRIVE',29,'Lleva los planos al observatorio.'),
 S('TUNE',29,'Marca la frecuencia de emergencia.',92))
mission('La tercera llave','Sara',
 'Para copiar la llave oriental necesito sostener la conexion desde fuera. Sara abrira el terminal durante su descanso.',
 'La tercera llave esta completa. Pero Sara no responde al despedirse. Solo se escucha una puerta metalica.',
 S('CIRCUIT',19,'Conecta la antena oriental.',132),
 S('TUNE',19,'Mantiene el enlace con Sara.',119),
 S('MEMORY',19,'Reconstruye la llave oriental.',104),
 S('CODE',19,'Valida las tres partes del registro.',967))
mission('Puertas de vidrio','Nico',
 'No voy a dejarla ahi. Luna encuentra una entrada de servicio. Vera me pide que piense antes de correr: sin las pruebas, Sara habra arriesgado todo para nada.',
 'Llegamos tarde. Sara dejo su chaqueta y una nota: Me llevan al Central. Todavia tengo el original.',
 S('DRIVE',11,'Llega a la torre con un carro.'),
 S('LOCK',11,'Abre la entrada de mantenimiento.',59),
 S('STEALTH',11,'Atraviesa el piso de seguridad.',117),
 S('TALK',11,'Busca a Sara en la sala borrada.'))
mission('Desviar la tormenta','Mara',
 'Prisma cierra las salidas del Centro. Mara propone una transmision falsa para que muevan sus equipos. Necesita tiempo y una senal convincente.',
 'El desvio funciona. Tenemos una ventana para evacuar los barrios antes del borrado.',
 S('TUNE',3,'Encuentra una portadora libre.',141),
 S('RHYTHM',3,'Compone el patron de la transmision senal.',23),
 S('CHASE',22,'Atrae la persecucion lejos del hospital y escapa.',5,26),
 S('TALK',14,'Confirma que la evacuacion puede empezar.'))
mission('Ruta de salida','Dona Luz',
 'No todos pueden dejar sus casas. Luz organiza alimentos y Luna prepara carros. Yo tengo que conectar los seis puntos de encuentro.',
 'Los puntos estan listos. Tiza pinta las rutas sobre el suelo. Esta vez la ciudad puede leer sus propias salidas.',
 S('DRIVE',12,'Recoge los suministros en el mercado.'),
 S('RACE',12,'Conecta seis puntos de encuentro.',17,120),
 S('CIRCUIT',14,'Reactiva la reserva electrica del hospital.',171),
 S('TALK',6,'Confirma las rutas con Tiza.'))
mission('El expediente completo','Vera',
 'Tenemos tres llaves y cientos de nombres. Para publicar las pruebas debemos reconstruir un expediente que cualquiera pueda verificar.',
 'Por primera vez la historia esta completa. Cardenal recibe el aviso judicial y adelanta Horizonte a esta misma noche.',
 S('MEMORY',5,'Ordena los registros de los tres nodos.',151),
 S('CODE',5,'Desbloquea el expediente completo.',358),
 S('DRIVE',2,'Entrega el expediente a Vera.'),
 S('TALK',2,'Decidimos actuar esta noche. Fin del capitulo IV.'))

mission('Hora cero','Luna',
 'La luz cae por sectores. No queda tiempo para entrar con cuidado. Luna prepara el carro, Mara abre la radio y Vera avisa a los vecinos.',
 'El Centro queda conectado a la radio del barrio. Aunque apaguen la red, podremos hablar.',
 S('TALK',1,'Reunete con Luna y revisa el carro.'),
 S('RACE',1,'Activa seis repetidores de emergencia.',22,105),
 S('TUNE',22,'Sincroniza la Plaza de la Luz.',176),
 S('CIRCUIT',15,'Sostiene la energia del Centro.',198))
mission('Las voces que faltan','Mara',
 'La radio recibe peticiones de ayuda y testimonios. Entre las voces aparece la de Sara. Esta dentro del Servidor Central y aun puede guiarnos.',
 'Sara revela el acceso trasero. Tambien nos advierte: destruir el servidor borraria las pruebas. Hay que copiar antes de detenerlo.',
 S('TUNE',3,'Separa la voz de Sara del ruido.',211),
 S('MEMORY',3,'Memoriza sus instrucciones de acceso.',177),
 S('DRIVE',25,'Lleva una bateria al Jardin del Sur.'),
 S('TALK',25,'Los vecinos abren una ruta hacia el Central.'))
mission('Cruzar la linea','Nico',
 'El camino del Sur esta abierto, pero Prisma lo vigila. El plan depende de llegar con el disco de Vera intacto.',
 'Llegamos al acceso del Central. Dentro, Horizonte ya empezo a borrar los registros mas antiguos.',
 S('DRIVE',23,'Lleva el disco al Servidor Central.'),
 S('CHASE',23,'Despista el ultimo seguimiento.',6,28),
 S('STEALTH',23,'Cruza el corredor de acceso.',208),
 S('LOCK',23,'Abre el cuarto de mantenimiento.',83))
mission('Al otro lado','Sara',
 'Sara responde detras de una puerta. Para abrirla debo reconstruir el circuito que Prisma desarmo. La escucho contar el tiempo entre cada corte.',
 'Sara sale. Esta agotada, pero se niega a marcharse sin copiar los nombres que todavia quedan en la maquina.',
 S('CIRCUIT',23,'Reconstruye la alimentacion de la puerta.',222),
 S('CODE',23,'Introduce la clave que Sara dejo dividida en pistas.',816),
 S('MEMORY',23,'Repite la secuencia de desbloqueo.',212),
 S('TALK',23,'Habla con Sara, cara a cara.'))
mission('No borres sus nombres','Sara',
 'La copia necesita una alimentacion estable. Si fallamos tendremos que intentarlo de nuevo mientras el sistema sigue cerrando accesos.',
 'La copia termina. Miles de firmas vuelven a tener dueno. Cardenal huye de la torre mientras Horizonte se queda sin ordenes.',
 S('CIRCUIT',23,'Aisla la copia del circuito de borrado.',251),
 S('TUNE',23,'Estabiliza el canal de transferencia.',246),
 S('RHYTHM',23,'Mantiene sincronizada la transferencia.',31),
 S('CODE',23,'Sella el respaldo con la clave final.',492))
mission('Amanecer pendiente','Luna',
 'La salida esta bloqueada. Luna llega desde el Sur, pero necesitamos llevar el disco a la radio antes de que corten la ultima antena.',
 'Mara conecta el disco. Tenemos las pruebas y a Sara de vuelta. Falta decidir que hacer con todo lo que encontramos.',
 S('DRIVE',3,'Lleva a Sara y el respaldo a Radio Ladera.'),
 S('CHASE',3,'Escapa del ultimo cerco.',7,30),
 S('CIRCUIT',3,'Devuelve la energia a la radio.',283),
 S('TALK',3,'Reune al grupo. Fin del capitulo V.'))

mission('Lo que queda','Vera',
 'La caida de Prisma no devuelve automaticamente las casas. Cada familia necesita sus documentos. Podemos ayudar a reconstruirlos.',
 'Los primeros expedientes vuelven a sus duenos. Luz abre el mercado otra vez, esta vez con una copia de seguridad en papel.',
 S('MEMORY',5,'Reconstruye los expedientes recuperados.',247),
 S('DRIVE',12,'Entrega las primeras carpetas a Dona Luz.'),
 S('CODE',12,'Recupera los registros del mercado.',632),
 S('TALK',12,'Escucha a los vecinos.'))
mission('Calles con memoria','Tiza',
 'Tiza quiere pintar los nombres de quienes defendieron sus barrios. No como heroes: como vecinos que decidieron quedarse juntos.',
 'El nuevo mural no tapa el anterior. Lo rodea. La primera llave sigue ahi, para que nadie olvide como empezo todo.',
 S('DRIVE',6,'Lleva pintura y bocetos a la cancha.'),
 S('RHYTHM',6,'Acompana el ritmo de la jornada del mural.',37),
 S('RACE',6,'Lleva copias del mural a seis barrios.',28,135),
 S('TALK',6,'Mira el mural con Tiza.'))
mission('Una ultima senal','Sara',
 'Queda una copia de Horizonte en la antena oriental. Ya no controla la ciudad, pero podria usarse otra vez si alguien la encuentra.',
 'El ultimo respaldo queda bloqueado y documentado. Sara entrega las llaves a una red de vecinos, para que nadie tenga todas a la vez.',
 S('DRIVE',19,'Acompana a Sara a la antena.'),
 S('STEALTH',19,'Evita los sensores que siguen activos.',279),
 S('CIRCUIT',19,'Desconecta el ultimo respaldo.',317),
 S('CODE',19,'Revoca las credenciales de Horizonte.',759))
mission('El sonido del regreso','Mara',
 'Mara organiza un concierto abierto. No hay lista de invitados ni equipos de Prisma. La ciudad vuelve a escucharse a si misma.',
 'Luna baila por primera vez en semanas. Sara se rie. Vera apaga el telefono. Yo me quedo un rato sin pensar en la siguiente entrega.',
 S('DRIVE',26,'Recoge los equipos del estudio.'),
 S('TUNE',22,'Prepara la emision desde la plaza.',292),
 S('RHYTHM',22,'Toca la primera sesion del concierto.',43),
 S('RHYTHM',22,'Cierra el concierto con el segundo tema.',49))
mission('La decision','Vera',
 'Tenemos pruebas publicas y datos privados de los vecinos. Vera propone llevar el expediente a la justicia. Mara pide publicar tambien una version protegida para todos.',
 'Nadie recuperara esas semanas. Pero podemos decidir como se cuenta la historia y quien conserva el control de sus datos.',
 S('TALK',2,'Escucha a Vera: un expediente verificable para la justicia.'),
 S('TALK',3,'Escucha a Mara: una memoria publica que proteja a los vecinos.'),
 S('MEMORY',5,'Separa los nombres privados de las pruebas publicas.',302),
 S('CHOICE',10,'Decide como compartir las pruebas desde el mirador.'))
mission('Medellin despierta','Nico',
 'La ciudad no se arreglo en una noche. Hay trabajo, calles por recorrer y deudas que no caben en un archivo. Pero el amanecer ya no pertenece a Prisma.',
 'Sara vuelve al mirador. Luna me devuelve las llaves del carro. Mara enciende la radio. Vera pregunta que hare ahora. Sonrio: primero, una vuelta por la ciudad.',
 S('DRIVE',1,'Visita a Luna en el taller.'),
 S('TALK',12,'Pasa por el mercado de Dona Luz.'),
 S('RHYTHM',3,'Graba el tema final en Radio Ladera.',55),
 S('DRIVE',10,'Conduce hasta el mirador para ver el amanecer.'),
 S('ENDING',10,'Habla con Sara. Despues podras seguir explorando.'))

if __name__=='__main__':
 root=pathlib.Path(__file__).resolve().parents[1]
 def q(s): return json.dumps(s,ensure_ascii=True)
 lines=['/* Generated by tools/story.py. Original fictional story. */',
 'static const Location locations[] = {']
 for name,x,y in LOCATIONS: lines.append('{%s,%d,%d},'%(q(name),x*320+62,y*320+62))
 lines+=['};','static const Mission missions[] = {']
 for m in M:
  steps=','.join('{K_%s,%d,%d,%d,%s}'%(s['kind'],s['loc'],s['seed'],s['par'],q(s['text'])) for s in m['steps'])
  lines.append('{%s,%s,%s,%s,%d,%d,{%s}},'%(q(m['title']),q(m['who']),q(m['intro']),q(m['outro']),len(m['steps']),m['reward'],steps))
 lines+=['};','static const char *chapters[] = {"I / FIRMAS AJENAS","II / TRES NODOS","III / EL RIO OCULTO","IV / HORIZONTE","V / HORA CERO","VI / LA CIUDAD ES NUESTRA"};']
 (root/'src/story.h').write_text('\n'.join(lines)+'\n')
 (root/'CAMPANA.json').write_text(json.dumps({'locations':LOCATIONS,'missions':M},indent=2,ensure_ascii=False))
 print(len(M),'missions;',sum(len(m['steps']) for m in M),'objectives')
