import java.io.*;
import java.net.*;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicBoolean;

public class Prog1Matricielle {
    static final int MON_ID = 1;
    static final int PORT1 = 6001, PORT2 = 6002, PORT3 = 6003, PORT4 = 6004;
    static final int NB_PROC = 4;
    static final String IP = "127.0.0.1";
    static final AtomicBoolean stop = new AtomicBoolean(false);
    static ServerSocket serverSocket;

    static int[][] matrixClock = new int[NB_PROC][NB_PROC];

    static void printMatrix(int[][] matrix) {
        for (int[] row : matrix) {
            System.out.println(Arrays.toString(row));
        }
    }
    static String matrixToString(int[][] matrix) {
        StringBuilder sb = new StringBuilder();
        for (int[] row : matrix) {
            for (int val : row) {
                sb.append(String.format("%3d ", val));
            }
            sb.append("\n");
        }
        return sb.toString();
    }
    
    static void sendToGUI(String log) {
        try (Socket guiSocket = new Socket("127.0.0.1", 7003); // chaque processus utilise son propre port
             PrintWriter out = new PrintWriter(guiSocket.getOutputStream(), true)) {
                out.println(log);
                } catch (IOException e) {
            System.out.println("Impossible d’envoyer au GUI : " + e.getMessage());
        }
    }

    static void displayClock(String event) {
        System.out.printf("[P%d] %s\nHorloge matricielle :\n", MON_ID, event);
        printMatrix(matrixClock);
        System.out.println();

        //pour l'interface
        String log = String.format("[P%d] %s\nHorloge matricielle :\n%s", MON_ID, event, matrixToString(matrixClock));
        sendToGUI(log);
    }

    static void updateOnReceive(MessageMatriciel msg) {
        System.out.printf("[P%d] Recu de P%d | Matrice recue :\n", MON_ID, msg.senderId);
        printMatrix(msg.matrix);

         //pour l'interface
         String log1 = String.format("[P%d] Recu de P%d | Matrice recue :\n%s", MON_ID, msg.senderId, matrixToString(msg.matrix));
         sendToGUI(log1);
     

        for (int j = 0; j < NB_PROC; j++) {
            for (int k = 0; k < NB_PROC; k++) {
                matrixClock[j][k] = Math.max(matrixClock[j][k], msg.matrix[j][k]);
            }
        }
        matrixClock[MON_ID - 1][MON_ID - 1]++;

        System.out.printf("[P%d] Nouvelle horloge :\n", MON_ID);
        printMatrix(matrixClock);
        System.out.println();
       
        //interface
        String log2 = String.format("[P%d] Nouvelle horloge :\n%s", MON_ID, matrixToString(matrixClock));
        sendToGUI(log2);
    }

    static void sendMessage(int port, int destId) {
        try (Socket socket = new Socket()) {
            socket.connect(new InetSocketAddress(IP, port), 2500);
            ObjectOutputStream out = new ObjectOutputStream(socket.getOutputStream());

            matrixClock[MON_ID - 1][MON_ID - 1]++; // evenement local
            matrixClock[MON_ID - 1][destId - 1]++;  // communication explicite

            MessageMatriciel msg = new MessageMatriciel(MON_ID, matrixClock);

            System.out.printf("[P%d] Envoi a P%d | Matrice envoyee :\n", MON_ID, destId);
            printMatrix(msg.matrix);
            System.out.println();
            out.writeObject(msg);
            
        //pour l'interface
        String log = String.format("[P%d] Envoi a P%d | Matrice envoyee :\n%s", MON_ID, destId, matrixToString(msg.matrix));
        sendToGUI(log);

            
        } catch (IOException e) {
            System.out.printf("[P%d] Connexion echouee au port %d (%s)\n", MON_ID, port, e.getMessage());
        }
    }

    static class Receiver extends Thread {
        public void run() {
            try {
                serverSocket = new ServerSocket(PORT1);
                System.out.printf("[P%d] Serveur pret sur le port %d\n", MON_ID, PORT1);
                while (!stop.get()) {
                    try (Socket clientSocket = serverSocket.accept();
                         ObjectInputStream in = new ObjectInputStream(clientSocket.getInputStream())) {
                        MessageMatriciel msg = (MessageMatriciel) in.readObject();
                        updateOnReceive(msg);
                    } catch (Exception e) {
                        if (!stop.get())
                            System.out.printf("[P%d] Erreur reception : %s\n", MON_ID, e.getMessage());
                    }
                }
                serverSocket.close();
            } catch (IOException e) {
                System.out.printf("[P%d] Erreur serveur : %s\n", MON_ID, e.getMessage());
            }
        }
    }

    public static void main(String[] args) throws Exception {
        new Receiver().start();
        Thread.sleep(1000);

        matrixClock[MON_ID - 1][MON_ID - 1]++;
        displayClock("Evenement local 1 : Affichage");

        matrixClock[MON_ID - 1][MON_ID - 1]++;
        int x = 4; x++;
        displayClock("Evenement local 2 : Incrementation");

        matrixClock[MON_ID - 1][MON_ID - 1]++;
        System.out.printf("[P%d] Heure systeme : %s\n", MON_ID, new java.util.Date());
        displayClock("Evenement local 3 : Heure systeme");

        matrixClock[MON_ID - 1][MON_ID - 1]++;
        int y = x + 5;
        displayClock("Evenement local 4 : Addition");

        
        Thread.sleep(1000);
        matrixClock[MON_ID - 1][MON_ID - 1]++;
        displayClock("Evenement local 5 : Pause 1s");

        sendMessage(PORT2, 2); Thread.sleep(200);
        sendMessage(PORT3, 3); Thread.sleep(200);
        sendMessage(PORT4, 4); Thread.sleep(200);
        sendMessage(PORT2, 2);

        Thread.sleep(5000);
        stop.set(true);
        if (serverSocket != null && !serverSocket.isClosed()) serverSocket.close();

        System.out.println("Appuyez sur Entrée pour quitter...");
        System.in.read();
    }
}