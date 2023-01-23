class CanceledFetchError extends Error {
    constructor(message) {
        super(message);
        this.name = CanceledFetchErrorName;
    }
}

export const CanceledFetchErrorName = 'ReactNativeBlobUtilCanceledFetch'

export default CanceledFetchError