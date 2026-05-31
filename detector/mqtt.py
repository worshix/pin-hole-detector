import paho.mqtt.client as mqtt
from django.conf import settings


def publish_control(command):
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.connect_timeout = settings.MQTT_CONNECT_TIMEOUT
    client.connect(settings.MQTT_BROKER_HOST, settings.MQTT_BROKER_PORT, keepalive=10)
    client.loop_start()
    result = client.publish(settings.MQTT_CONTROL_TOPIC, command, qos=1, retain=True)
    result.wait_for_publish(timeout=settings.MQTT_CONNECT_TIMEOUT)
    client.loop_stop()
    client.disconnect()

    if not result.is_published():
        raise TimeoutError(
            f'Could not publish to {settings.MQTT_BROKER_HOST}:{settings.MQTT_BROKER_PORT} '
            f'on topic {settings.MQTT_CONTROL_TOPIC}'
        )
