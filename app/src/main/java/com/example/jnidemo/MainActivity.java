package com.example.jnidemo;

import androidx.appcompat.app.AppCompatActivity;

import android.graphics.Color;
import android.os.Bundle;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    public native boolean isDebugDetected();
    public native String helloFromJNI();
    public native int factorial(int n);

    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        TextView tvStatus = findViewById(R.id.tvStatus);
        TextView tvHello = findViewById(R.id.tvHello);
        TextView tvFact = findViewById(R.id.tvFact);

        boolean suspicious = isDebugDetected();

        if (suspicious) {
            tvStatus.setText("Etat securite : environnement suspect detecte");
            tvStatus.setTextColor(Color.RED);

            tvHello.setText("Fonction native sensible desactivee");
            tvFact.setText("Calcul natif bloque");
        } else {
            tvStatus.setText("Etat securite : OK");
            tvStatus.setTextColor(Color.parseColor("#2E7D32"));

            tvHello.setText(helloFromJNI());

            int result = factorial(10);
            if (result >= 0) {
                tvFact.setText("Factoriel de 10 = " + result);
            } else {
                tvFact.setText("Erreur factoriel");
            }
        }
    }
}
