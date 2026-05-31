from django.urls import path

from . import views

urlpatterns = [
    path('', views.dashboard, name='dashboard'),
    path('history/', views.history, name='history'),
    path('control/<str:command>/', views.control_machine, name='control_machine'),
    path('api/images/receive/', views.receive_image, name='receive_image'),
]
