import java.io.Serializable;

public class MessageMatriciel implements Serializable {
    public int senderId;
    public int[][] matrix;

    public MessageMatriciel(int senderId, int[][] matrix) {
        this.senderId = senderId;
        this.matrix = new int[matrix.length][matrix[0].length];
        for (int i = 0; i < matrix.length; i++) {
            System.arraycopy(matrix[i], 0, this.matrix[i], 0, matrix[i].length);
        }
    }
}
