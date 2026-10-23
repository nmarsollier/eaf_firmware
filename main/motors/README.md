# Motor paso a paso YEJMKJ NEMA 11

Fuente: [Publicación de Amazon](https://www.amazon.com/dp/B0FHHSVXQ7)  
ASIN: B0FHHSVXQ7

Motor paso a paso bipolar de dos fases. Los siguientes valores corresponden a las especificaciones publicadas por el vendedor.

## Especificaciones técnicas

| Parámetro | Valor |
|---|---|
| Marca | YEJMKJ |
| Formato | NEMA 11 |
| Tipo | Paso a paso bipolar |
| Fases | 2 |
| Conductores | 4 |
| Paso angular | 1,8° |
| Resolución | 200 pasos/vuelta |
| Torque de retención | 8 N·cm = 0,08 N·m ≈ 0,816 kgf·cm |
| Corriente nominal | 0,8 A |
| Resistencia de fase | 6,1 Ω |
| Inductancia | 4,2 mH ±20 % |
| Inercia del rotor | Aproximadamente 8,5 g·cm² |
| Dimensiones del cuerpo | 28 × 28 × 30,8 mm |
| Diámetro del eje | 5 mm |
| Longitud del eje | 20 ±0,5 mm |
| Longitud del plano del eje tipo D | 15 mm |
| Separación de montaje | 23 mm |
| Peso | 115 g |
| Longitud del cable | 1 m |

## Conexionado

| Bobina | Terminal | Color |
|---|---|---|
| A | A+ | Negro |
| A | A− | Verde |
| B | B+ | Rojo |
| B | B− | Azul |

## Condiciones ambientales y aislamiento

| Parámetro | Valor |
|---|---|
| Temperatura ambiente de operación | −20 a +50 °C |
| Humedad relativa | 15–95 % |
| Elevación térmica máxima | 80 K |
| Clase de aislamiento | B, 130 °C |
| Resistencia de aislamiento | ≥100 MΩ |
| Rigidez dieléctrica | 500 V CA durante 1 minuto |

## Alimentación y velocidad

La sección general de la marca recomienda:

- Alimentación: 12–24 V CC mediante un controlador con limitación de corriente.
- Velocidad: 0–600 rpm.

Estas recomendaciones son generales. La publicación no presenta una curva torque–velocidad específica que garantice ese rango bajo carga.

No conectar directamente las bobinas a 12–24 V sin regulación de corriente.

## Cálculos derivados

Estos valores son cálculos a partir de las especificaciones, no prestaciones adicionales garantizadas por el fabricante.

### Tensión resistiva de una bobina

V = I × R = 0,8 A × 6,1 Ω = 4,88 V

Esta tensión no debe confundirse con la alimentación del controlador.

### Disipación resistiva

Por bobina, a 0,8 A:

P = I² × R = 0,8² × 6,1 ≈ 3,90 W

Con ambas bobinas simultáneamente a esa corriente:

P_total ≈ 7,81 W

Es disipación térmica en los bobinados, no potencia mecánica de salida. El valor real depende de cómo el controlador excite las fases.

### Resolución con microstepping

Ejemplo con un controlador configurado a 1/16 de paso:

- 200 × 16 = 3.200 micropasos/vuelta.
- 1,8° / 16 = 0,1125° nominales por micropaso.

La subdivisión en micropasos no garantiza una precisión mecánica equivalente.

## Interpretación del torque

El torque de retención de 0,08 N·m corresponde al motor energizado y detenido.

No debe interpretarse como torque disponible a cualquier velocidad. Para dimensionar una aplicación en movimiento hace falta la curva torque–velocidad.

## Información no confirmada

No encontré una ficha técnica independiente del fabricante que confirme:

- Curva torque–velocidad.
- Precisión angular.
- Cargas radiales y axiales admisibles.
- Rosca y profundidad de los agujeros de montaje.
- Grado de protección IP.
- Vida útil.