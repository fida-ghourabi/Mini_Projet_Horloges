import java.io.Serializable;

public class MessageVectoriel implements Serializable {
    public int senderId;
    public int[] vector;

    public MessageVectoriel(int senderId, int[] vector) {
        this.senderId = senderId;
        this.vector = vector.clone(); // deep copy
    }
}

